#!/usr/bin/env python3
"""Run UPC track vertexing QA and save histograms (no AOD writer)."""
import argparse
import contextlib
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

SERVICES = [
    'o2-analysis-event-selection-service',
    'o2-analysis-trackselection',
    'o2-analysis-pid-tpc-service',
    'o2-analysis-propagationservice',
]


def positive_int(value):
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError('must be positive')
    return number


def positive_float(value):
    number = float(value)
    if not 0 < number < float('inf'):
        raise argparse.ArgumentTypeError('must be positive and finite')
    return number


def input_entry(value, base):
    if '://' in value:
        return value
    path = Path(value).expanduser()
    return str((base / path).resolve()) if not path.is_absolute() else str(path)


def pipeline(args, config, data):
    common = ['--configuration', 'json://' + str(config),
              '--shm-segment-size', str(args.shm_segment_size), '-b',
              '--readers', str(args.readers), '--time-limit', str(args.time_limit),
              '--aod-file', data]
    commands = [[executable] + common for executable in [args.script] + args.services]
    commands[-1] += ['--fairmq-ipc-prefix', '.']
    return ' | '.join(' '.join(shlex.quote(part) for part in cmd) for cmd in commands)


def run_job(args, config, data, output, log):
    command = pipeline(args, config, data)
    print('\nAnalysis: ' + command, flush=True)
    print('Output: ' + str(output), flush=True)
    if args.dry_run:
        return
    # Isolate default outputs and IPC files, including each chunk's output.
    with tempfile.TemporaryDirectory(prefix='upc-qa-', dir=Path.cwd()) as work:
        subprocess.run(['bash', '-o', 'pipefail', '-c', command], cwd=work,
                       stdout=log, stderr=subprocess.STDOUT if log else None,
                       check=True)
        result = Path(work) / 'AnalysisResults.root'
        if not result.is_file():
            raise RuntimeError('Analysis completed without producing AnalysisResults.root')
        output.parent.mkdir(parents=True, exist_ok=True)
        result.replace(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-j', '--json', dest='json_file', required=True,
                        help='Required JSON configuration')
    parser.add_argument('-d', '--data', required=True,
                        help='Required AO2D.root, remote URI, or @file_list.txt (plain .txt also accepted)')
    parser.add_argument('-s', '--script', default='o2-analysis-ud-upc-track-vertexing-qa')
    parser.add_argument('--sw', dest='services', nargs='+', default=SERVICES,
                        help='Replace the default four services')
    parser.add_argument('--shm-segment-size', type=positive_int, default=1000000000000)
    parser.add_argument('--readers', type=positive_int, default=1)
    parser.add_argument('--time-limit', type=positive_int, default=500)
    parser.add_argument('-o', '--output', default='AnalysisResults_jpsi_emb.root')
    parser.add_argument('-n', '--dry-run', action='store_true')
    parser.add_argument('-l', '--log')
    parser.add_argument('-u', '--use-sub-jobs', action='store_true')
    chunk = parser.add_mutually_exclusive_group()
    chunk.add_argument('--chunk-num', type=positive_int, help='Files per chunk (default: 2)')
    chunk.add_argument('--chunk-max-size', type=positive_float,
                       help='Maximum GiB per chunk; local inputs only; oversized files run alone')
    parser.add_argument('--jobs-dir', default='jobs')
    args = parser.parse_args()
    if not args.use_sub_jobs and (args.chunk_num or args.chunk_max_size or args.jobs_dir != 'jobs'):
        parser.error('chunk options and --jobs-dir require --use-sub-jobs')
    config = Path(args.json_file).expanduser().resolve()
    output = Path(args.output).expanduser().resolve()
    try:
        with config.open() as handle:
            settings = json.load(handle)
        if not isinstance(settings, dict):
            parser.error('JSON configuration must contain an object')
        is_list = args.data.startswith('@') or args.data.lower().endswith('.txt')
        if is_list:
            source = Path(args.data.lstrip('@')).expanduser().resolve()
            # Resolve list entries relative to the supplied list, before changing cwd.
            entries = [input_entry(line.strip(), source.parent)
                       for line in source.read_text().splitlines()
                       if line.strip() and not line.lstrip().startswith('#')]
            if not entries:
                parser.error('input file list is empty')
        else:
            entries = [input_entry(args.data, Path.cwd())]
        for entry in entries:
            if '://' not in entry and not Path(entry).is_file():
                parser.error('input file not found: ' + entry)
        if args.use_sub_jobs and not is_list:
            parser.error('--use-sub-jobs requires a text file list')
        if args.use_sub_jobs and args.chunk_max_size:
            chunks, current, size = [], [], 0
            for entry in entries:
                if '://' in entry:
                    parser.error('--chunk-max-size requires local files; use --chunk-num for remote files')
                entry_size = Path(entry).stat().st_size
                if current and size + entry_size > args.chunk_max_size * 1024 ** 3:
                    chunks.append(current)
                    current, size = [], 0
                current.append(entry)
                size += entry_size
            if current:
                chunks.append(current)
        elif args.use_sub_jobs:
            count = args.chunk_num or 2
            chunks = [entries[i:i + count] for i in range(0, len(entries), count)]
        else:
            chunks = [entries]
        if args.dry_run:
            log_context = contextlib.nullcontext(None)
        else:
            log_context = open(args.log, 'w') if args.log else contextlib.nullcontext(None)
        with log_context as log:
            def execute():
                # A normalized copy ensures data always comes from --data.
                # The supplied JSON is never modified.
                with tempfile.TemporaryDirectory(prefix='upc-input-') as temp:
                    run_config = Path(temp) / config.name
                    reader = settings.get('internal-dpl-aod-reader', {})
                    if not isinstance(reader, dict):
                        raise ValueError('internal-dpl-aod-reader must be an object')
                    reader.pop('aod-file', None)
                    reader.pop('aod-file-private', None)
                    run_config.write_text(json.dumps(settings, indent=2))
                    results = []
                    jobs_dir = Path(args.jobs_dir).expanduser().resolve()
                    for index, group in enumerate(chunks, 1):
                        if is_list:
                            file_list = Path(temp) / ('chunk-%d.txt' % index)
                            file_list.write_text('\n'.join(group) + '\n')
                            data = '@' + str(file_list)
                        else:
                            data = group[0]
                        target = jobs_dir / ('AnalysisResults-job-%d.root' % index) if args.use_sub_jobs else output
                        if args.use_sub_jobs and target == output:
                            raise ValueError('--output must differ from individual sub-job outputs')
                        run_job(args, run_config, data, target, log)
                        results.append(target)
                    if args.use_sub_jobs:
                        command = ['hadd', '-f', str(output)] + [str(path) for path in results]
                        print('\nMerge histograms: ' + ' '.join(map(shlex.quote, command)), flush=True)
                        if not args.dry_run:
                            output.parent.mkdir(parents=True, exist_ok=True)
                            subprocess.run(command, stdout=log,
                                           stderr=subprocess.STDOUT if log else None, check=True)
                    print('\n' + ('Dry run completed; no analysis executed.' if args.dry_run else 'Saved: ' + str(output)))
            if log:
                with contextlib.redirect_stdout(log), contextlib.redirect_stderr(log):
                    execute()
            else:
                execute()
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print('Error: ' + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())

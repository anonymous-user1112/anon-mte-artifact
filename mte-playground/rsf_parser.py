#!/usr/bin/env python3

import os
import argparse
import datetime
import json
import socket
import platform
import statistics

script_folder = os.path.dirname(os.path.realpath(__file__))

def find(name, path):
    for root, dirs, files in os.walk(path):
        if name in files:
            return os.path.join(root, name)

    return None

class RsfBase:
    def __init__(self, prefix, lines):
        self.prefix = prefix
        self.lines = RsfBase.strip_lines(prefix, lines)

    @staticmethod
    def strip_lines(prefix, lines):
        stripped = []

        for line in lines:
            assert line.startswith(prefix)
            stripped.append(line[len(prefix) + 1:])

        return stripped

    def get_lines(self, key, lines = None):
        if lines == None:
            lines = self.lines
        return [x for x in lines if x.startswith(key)]

    def get_values(self, key, lines = None):
        return [x.split(':')[-1].strip() for x in self.get_lines(key, lines)]

    def get_value(self, key, lines = None):
        values = self.get_values(key, lines)
        assert len(values) == 1
        return values[0]

    def get_value_or_default(self, key, lines = None):
        values = self.get_values(key, lines)
        return values[0] if len(values) > 0 else None

class Benchmark(RsfBase):
    def __init__(self, name, lines):
        self.name = name

        super(Benchmark, self).__init__('.'.join([name, 'base']), lines)
        runs = sorted(set([x.split('.')[0] for x in self.lines]))
        self.errors = []
        self.times = []
        self.iterations = len(runs)

        for run in runs:
            lines = RsfBase.strip_lines(run, [x for x in self.lines if x.startswith(run)])
            t = self.get_value('reported_time', lines)
            self.times.append(float(t) if t != '--' else None)
            self.errors += self.get_values('error', lines)

        # parse ELF sections
    def to_dict(self):
        return { 'name': self.name,
                'times': self.times,
                'iterations': self.iterations,
                'errors': ''.join(self.errors)
                }

# one file
class BenchmarkGroup(RsfBase):
    def __init__(self, filename, spec_name):
        self.filename = filename
        lines = [x.strip() for x in open(self.filename).readlines()]
        super(BenchmarkGroup, self).__init__('spec.' + spec_name, [x for x in lines if not x.startswith('#')])

        benchmark_lines = RsfBase.strip_lines('results', self.get_lines('results'))
        benchmark_names = sorted(set([x.split('.')[0] for x in benchmark_lines]))
        self.benchmarks = [Benchmark(x, [y for y in benchmark_lines if y.startswith(x)]) for x in benchmark_names]
        self.unitbase = self.get_value_or_default('unitbase')
        if self.unitbase == None:
            self.unitbase = self.get_value('name')

    def to_dict(self):
        return { 'group_name': self.unitbase, 'benchmarks': [x.to_dict() for x in self.benchmarks] }

# one set of INT (and FP)
class BenchmarkSuite(RsfBase):
    def __init__(self, filenames, spec_name):
        lines = [x.strip() for x in open(filenames[0]).readlines()]
        super(BenchmarkSuite, self).__init__('spec.' + spec_name, [x for x in lines if not x.startswith('#')])
        self.time = self.get_value('time')
        self.time = datetime.datetime.fromtimestamp(float(self.time))
        self.toolset = self.get_value('toolset')
        self.suitever = self.get_value('suitever')
        self.suitename = spec_name

        self.groups = [BenchmarkGroup(x, spec_name) for x in filenames]

    @staticmethod
    def from_log(log_file, suite_name="cpu2006"):
        dirpath = os.path.dirname(log_file)
        lines = [x.strip() for x in open(log_file).readlines()]

        p = 'format: raw -> '
        rsf_files = [os.path.join(dirpath, os.path.basename(x.split(p)[-1].strip())) for x in lines if p in x]
        assert len(rsf_files) > 0
        print("rsf output path : ", rsf_files)
        return BenchmarkSuite(rsf_files, suite_name)

    def to_dict(self):
        return {
            'timestamp': self.time.strftime('%Y-%m-%dT%H:%M:%S'),
            'type': self.suitename,
            'groups': [x.to_dict() for x in self.groups],
            # 'toolset': self.toolset,
            # 'arch': platform.machine(),
            # 'hostname': socket.gethostname(),
            }

def main():
    parser = argparse.ArgumentParser(description='Parse SPEC RSF file and transform selected values to JSON')
    parser.add_argument('log_file', metavar = 'log_file', help = 'SPEC log output file')
    parser.add_argument("-o", "--output", dest="output", help = "JSON output file")

    args = parser.parse_args()

    suite = BenchmarkSuite.from_log(args.log_file)

    if args.output == None:
        print(json.dumps(suite.to_dict(), indent = 2))
    else:
        with open(args.output, 'w') as fp:
            json.dump(suite.to_dict(), fp, indent = 2)


if __name__ == "__main__":
    main()

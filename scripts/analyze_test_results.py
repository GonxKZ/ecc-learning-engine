#!/usr/bin/env python3
"""
ECScope Test Results Analyzer
Analyzes test execution results and generates comprehensive reports.
"""

import json
import os
import sys
import glob
import argparse
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Any, Optional
import statistics
import datetime

@dataclass
class TestResult:
    name: str
    status: str  # "passed", "failed", "skipped"
    duration_seconds: float
    category: str  # "unit", "integration", "performance", etc.
    output: str = ""
    error_message: str = ""

@dataclass
class BenchmarkResult:
    name: str
    average_time_ns: int
    min_time_ns: int
    max_time_ns: int
    std_deviation_ns: int
    iterations: int
    memory_usage_bytes: int = 0

@dataclass
class PerformanceMetrics:
    test_suite: str
    total_tests: int
    passed_tests: int
    failed_tests: int
    skipped_tests: int
    total_duration: float
    coverage_percentage: float = 0.0
    benchmarks: List[BenchmarkResult] = None

class TestResultsAnalyzer:
    def __init__(self, artifacts_dir: str):
        self.artifacts_dir = Path(artifacts_dir)
        self.results: Dict[str, PerformanceMetrics] = {}
        self.test_results: List[TestResult] = []
        self.benchmarks: List[BenchmarkResult] = []

    def analyze_artifacts(self):
        """Analyze all test artifacts in the directory."""
        print("Analyzing test artifacts...")
        
        # Find all test result files
        test_files = list(self.artifacts_dir.glob("**/test_results_*.json"))
        performance_files = list(self.artifacts_dir.glob("**/performance_report.json"))
        benchmark_files = list(self.artifacts_dir.glob("**/benchmark_results.json"))
        
        print(f"Found {len(test_files)} test result files")
        print(f"Found {len(performance_files)} performance report files")
        print(f"Found {len(benchmark_files)} benchmark result files")
        
        # Analyze test results
        for test_file in test_files:
            self.analyze_test_file(test_file)
        
        # Analyze performance results
        for perf_file in performance_files:
            self.analyze_performance_file(perf_file)
        
        # Analyze benchmark results
        for bench_file in benchmark_files:
            self.analyze_benchmark_file(bench_file)

    def analyze_test_file(self, test_file: Path):
        """Analyze a single test results file."""
        try:
            with open(test_file, 'r') as f:
                data = json.load(f)
            
            suite_name = data.get('suite_name', test_file.stem)
            
            metrics = PerformanceMetrics(
                test_suite=suite_name,
                total_tests=data.get('total_tests', 0),
                passed_tests=data.get('passed_tests', 0),
                failed_tests=data.get('failed_tests', 0),
                skipped_tests=data.get('skipped_tests', 0),
                total_duration=data.get('total_duration_seconds', 0.0),
                coverage_percentage=data.get('coverage_percentage', 0.0)
            )
            
            self.results[suite_name] = metrics
            
            # Extract individual test results
            for test_data in data.get('tests', []):
                test_result = TestResult(
                    name=test_data.get('name', ''),
                    status=test_data.get('status', 'unknown'),
                    duration_seconds=test_data.get('duration_seconds', 0.0),
                    category=test_data.get('category', 'unknown'),
                    output=test_data.get('output', ''),
                    error_message=test_data.get('error_message', '')
                )
                self.test_results.append(test_result)
            
        except Exception as e:
            print(f"Error analyzing test file {test_file}: {e}")

    def analyze_performance_file(self, perf_file: Path):
        """Analyze a performance report file."""
        try:
            with open(perf_file, 'r') as f:
                data = json.load(f)
            
            # Extract system information
            system_info = data.get('system_info', {})
            print(f"Performance test system: {system_info}")
            
            # Extract performance test results
            for test_result in data.get('test_results', []):
                bench_result = BenchmarkResult(
                    name=test_result.get('test_name', ''),
                    average_time_ns=test_result.get('average_time_ns', 0),
                    min_time_ns=test_result.get('min_time_ns', 0),
                    max_time_ns=test_result.get('max_time_ns', 0),
                    std_deviation_ns=test_result.get('std_deviation_ns', 0),
                    iterations=test_result.get('iterations', 0),
                    memory_usage_bytes=test_result.get('memory_usage_bytes', 0)
                )
                self.benchmarks.append(bench_result)
                
        except Exception as e:
            print(f"Error analyzing performance file {perf_file}: {e}")

    def analyze_benchmark_file(self, bench_file: Path):
        """Analyze a benchmark results file."""
        try:
            with open(bench_file, 'r') as f:
                data = json.load(f)
            
            for benchmark_data in data.get('benchmarks', []):
                bench_result = BenchmarkResult(
                    name=benchmark_data.get('name', ''),
                    average_time_ns=benchmark_data.get('average_time_ns', 0),
                    min_time_ns=benchmark_data.get('min_time_ns', 0),
                    max_time_ns=benchmark_data.get('max_time_ns', 0),
                    std_deviation_ns=benchmark_data.get('std_deviation_ns', 0),
                    iterations=benchmark_data.get('iterations', 0),
                    memory_usage_bytes=benchmark_data.get('memory_usage_bytes', 0)
                )
                self.benchmarks.append(bench_result)
                
        except Exception as e:
            print(f"Error analyzing benchmark file {bench_file}: {e}")

    def generate_summary(self) -> Dict[str, Any]:
        """Generate a comprehensive summary of all test results."""
        total_tests = sum(metrics.total_tests for metrics in self.results.values())
        total_passed = sum(metrics.passed_tests for metrics in self.results.values())
        total_failed = sum(metrics.failed_tests for metrics in self.results.values())
        total_skipped = sum(metrics.skipped_tests for metrics in self.results.values())
        
        # Calculate average coverage
        coverages = [m.coverage_percentage for m in self.results.values() if m.coverage_percentage > 0]
        avg_coverage = statistics.mean(coverages) if coverages else 0.0
        
        # Analyze failure patterns
        failed_tests = [t for t in self.test_results if t.status == "failed"]
        failure_categories = {}
        for test in failed_tests:
            failure_categories[test.category] = failure_categories.get(test.category, 0) + 1
        
        # Performance analysis
        performance_summary = {}
        if self.benchmarks:
            performance_summary = {
                'total_benchmarks': len(self.benchmarks),
                'fastest_test': min(self.benchmarks, key=lambda x: x.average_time_ns),
                'slowest_test': max(self.benchmarks, key=lambda x: x.average_time_ns),
                'average_execution_time_ns': statistics.mean([b.average_time_ns for b in self.benchmarks]),
                'total_memory_usage': sum([b.memory_usage_bytes for b in self.benchmarks])
            }
        
        return {
            'summary': {
                'total_tests': total_tests,
                'passed_tests': total_passed,
                'failed_tests': total_failed,
                'skipped_tests': total_skipped,
                'pass_rate': (total_passed / total_tests * 100) if total_tests > 0 else 0,
                'average_coverage': avg_coverage
            },
            'test_suites': {name: {
                'total': metrics.total_tests,
                'passed': metrics.passed_tests,
                'failed': metrics.failed_tests,
                'duration': metrics.total_duration,
                'coverage': metrics.coverage_percentage
            } for name, metrics in self.results.items()},
            'failure_analysis': {
                'by_category': failure_categories,
                'failed_tests': [{
                    'name': t.name,
                    'category': t.category,
                    'error': t.error_message[:200] + '...' if len(t.error_message) > 200 else t.error_message
                } for t in failed_tests[:10]]  # Top 10 failures
            },
            'performance': performance_summary,
            'generated_at': datetime.datetime.now().isoformat()
        }

    def detect_performance_regressions(self, baseline_file: Optional[Path] = None) -> List[Dict]:
        """Detect performance regressions compared to baseline."""
        regressions = []
        
        if not baseline_file or not baseline_file.exists():
            print("No baseline file provided or file doesn't exist")
            return regressions
        
        try:
            with open(baseline_file, 'r') as f:
                baseline_data = json.load(f)
            
            baseline_benchmarks = {b['name']: b for b in baseline_data.get('benchmarks', [])}
            
            for current_bench in self.benchmarks:
                if current_bench.name in baseline_benchmarks:
                    baseline = baseline_benchmarks[current_bench.name]
                    baseline_time = baseline.get('average_time_ns', 0)
                    
                    if baseline_time > 0:
                        regression_ratio = current_bench.average_time_ns / baseline_time
                        
                        if regression_ratio > 1.15:  # 15% slower
                            regressions.append({
                                'test_name': current_bench.name,
                                'baseline_time_ns': baseline_time,
                                'current_time_ns': current_bench.average_time_ns,
                                'regression_ratio': regression_ratio,
                                'slowdown_percentage': (regression_ratio - 1.0) * 100
                            })
            
        except Exception as e:
            print(f"Error detecting performance regressions: {e}")
        
        return regressions

    def save_analysis(self, output_file: str):
        """Save the analysis results to a file."""
        summary = self.generate_summary()
        
        try:
            with open(output_file, 'w') as f:
                json.dump(summary, f, indent=2, default=str)
            print(f"Analysis saved to {output_file}")
        except Exception as e:
            print(f"Error saving analysis: {e}")

    def generate_markdown_summary(self) -> str:
        """Generate a markdown summary for GitHub comments."""
        summary = self.generate_summary()
        
        md = []
        md.append("## 📊 Test Results Summary\n")
        
        # Overall stats
        overall = summary['summary']
        md.append(f"- **Total Tests**: {overall['total_tests']}")
        md.append(f"- **Passed**: {overall['passed_tests']} ✅")
        md.append(f"- **Failed**: {overall['failed_tests']} ❌")
        md.append(f"- **Skipped**: {overall['skipped_tests']} ⏭️")
        md.append(f"- **Pass Rate**: {overall['pass_rate']:.1f}%")
        md.append(f"- **Average Coverage**: {overall['average_coverage']:.1f}%\n")
        
        # Test suites breakdown
        if summary['test_suites']:
            md.append("### Test Suites\n")
            for suite_name, suite_data in summary['test_suites'].items():
                status_icon = "✅" if suite_data['failed'] == 0 else "❌"
                md.append(f"- **{suite_name}** {status_icon}: {suite_data['passed']}/{suite_data['total']} "
                         f"({suite_data['duration']:.2f}s)")
            md.append("")
        
        # Performance summary
        if summary['performance']:
            perf = summary['performance']
            md.append("### 🚀 Performance Summary\n")
            md.append(f"- **Benchmarks**: {perf['total_benchmarks']}")
            if 'fastest_test' in perf:
                md.append(f"- **Fastest Test**: {perf['fastest_test'].name} "
                         f"({perf['fastest_test'].average_time_ns / 1_000_000:.2f}ms)")
            if 'slowest_test' in perf:
                md.append(f"- **Slowest Test**: {perf['slowest_test'].name} "
                         f"({perf['slowest_test'].average_time_ns / 1_000_000:.2f}ms)")
            md.append("")
        
        # Failures
        if summary['failure_analysis']['failed_tests']:
            md.append("### ❌ Failed Tests\n")
            for test in summary['failure_analysis']['failed_tests'][:5]:  # Top 5
                md.append(f"- **{test['name']}** ({test['category']})")
                if test['error']:
                    md.append(f"  ```\n  {test['error']}\n  ```")
            md.append("")
        
        return "\n".join(md)

def main():
    parser = argparse.ArgumentParser(description="Analyze ECScope test results")
    parser.add_argument("artifacts_dir", help="Directory containing test artifacts")
    parser.add_argument("--output", "-o", default="test_analysis.json", 
                       help="Output file for analysis results")
    parser.add_argument("--markdown", "-m", help="Generate markdown summary file")
    parser.add_argument("--baseline", "-b", help="Baseline file for performance regression detection")
    
    args = parser.parse_args()
    
    if not os.path.exists(args.artifacts_dir):
        print(f"Artifacts directory {args.artifacts_dir} does not exist")
        sys.exit(1)
    
    analyzer = TestResultsAnalyzer(args.artifacts_dir)
    analyzer.analyze_artifacts()
    
    # Save analysis
    analyzer.save_analysis(args.output)
    
    # Generate markdown summary
    if args.markdown:
        md_summary = analyzer.generate_markdown_summary()
        with open(args.markdown, 'w') as f:
            f.write(md_summary)
        print(f"Markdown summary saved to {args.markdown}")
    
    # Always save markdown for GitHub actions
    md_summary = analyzer.generate_markdown_summary()
    with open('test_summary.md', 'w') as f:
        f.write(md_summary)
    
    # Check for performance regressions
    if args.baseline:
        regressions = analyzer.detect_performance_regressions(Path(args.baseline))
        if regressions:
            print(f"\n⚠️  Found {len(regressions)} performance regressions:")
            for reg in regressions[:5]:  # Top 5
                print(f"  - {reg['test_name']}: {reg['slowdown_percentage']:.1f}% slower")
    
    # Print summary
    summary = analyzer.generate_summary()
    overall = summary['summary']
    print(f"\n📊 Summary: {overall['passed_tests']}/{overall['total_tests']} tests passed "
          f"({overall['pass_rate']:.1f}% pass rate)")
    
    # Exit with appropriate code
    if overall['failed_tests'] > 0:
        sys.exit(1)

if __name__ == "__main__":
    main()
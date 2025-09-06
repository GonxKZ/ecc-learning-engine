#!/usr/bin/env python3
"""
Performance Regression Detection Script for ECScope
Analyzes performance test results and detects regressions.
"""

import json
import argparse
import sys
from pathlib import Path
from typing import Dict, List, Any, Optional

class PerformanceRegressionChecker:
    def __init__(self, threshold: float = 0.05, fail_on_regression: bool = True):
        self.threshold = threshold
        self.fail_on_regression = fail_on_regression
        self.regressions = []
        
    def check_regressions(self, analysis_data: Dict[str, Any]) -> bool:
        """
        Check for performance regressions in the analysis data.
        
        Args:
            analysis_data: Performance analysis results
            
        Returns:
            bool: True if no significant regressions found
        """
        has_regressions = False
        
        print(f"🔍 Checking for performance regressions (threshold: {self.threshold:.1%})")
        print("=" * 60)
        
        # Check if analysis data contains comparisons
        if 'comparisons' in analysis_data:
            for test_name, comparison in analysis_data['comparisons'].items():
                regression_detected = self._check_single_test(test_name, comparison)
                if regression_detected:
                    has_regressions = True
                    
        elif 'benchmarks' in analysis_data:
            # Handle direct benchmark data
            for benchmark in analysis_data['benchmarks']:
                regression_detected = self._check_benchmark_regression(benchmark)
                if regression_detected:
                    has_regressions = True
        else:
            print("⚠️  No comparison data found in analysis")
            
        # Summary
        self._print_summary(has_regressions)
        
        return not has_regressions
        
    def _check_single_test(self, test_name: str, comparison: Dict[str, Any]) -> bool:
        """Check a single test for regressions."""
        if 'performance_change' not in comparison:
            return False
            
        change = comparison['performance_change']
        
        # Negative change means performance degradation
        if change < -self.threshold:
            regression_info = {
                'test': test_name,
                'change': change,
                'change_percent': change * 100,
                'severity': self._get_regression_severity(abs(change))
            }
            self.regressions.append(regression_info)
            
            severity_icon = "🔴" if abs(change) > 0.2 else "🟡"
            print(f"{severity_icon} REGRESSION in {test_name}: {change:.1%} slower")
            
            if 'current_value' in comparison and 'baseline_value' in comparison:
                current = comparison['current_value']
                baseline = comparison['baseline_value']
                print(f"   Current: {current:.3f}ms, Baseline: {baseline:.3f}ms")
                
            return True
        elif change > self.threshold:
            improvement_icon = "✅"
            print(f"{improvement_icon} IMPROVEMENT in {test_name}: {change:.1%} faster")
            
        return False
        
    def _check_benchmark_regression(self, benchmark: Dict[str, Any]) -> bool:
        """Check individual benchmark for regression patterns."""
        # This is a simplified check for when we don't have baseline comparison
        test_name = benchmark.get('name', 'unknown')
        
        # Look for warning indicators in the benchmark data
        if benchmark.get('error_occurred', False):
            print(f"⚠️  Error detected in {test_name}")
            return True
            
        if benchmark.get('timeout', False):
            print(f"⏰ Timeout detected in {test_name}")
            return True
            
        return False
        
    def _get_regression_severity(self, change_magnitude: float) -> str:
        """Determine regression severity based on magnitude."""
        if change_magnitude > 0.5:  # 50%+ slower
            return "critical"
        elif change_magnitude > 0.2:  # 20%+ slower
            return "major"
        elif change_magnitude > 0.1:  # 10%+ slower
            return "moderate"
        else:
            return "minor"
            
    def _print_summary(self, has_regressions: bool):
        """Print regression analysis summary."""
        print("\n" + "=" * 60)
        print("📊 REGRESSION ANALYSIS SUMMARY")
        print("=" * 60)
        
        if has_regressions:
            print(f"❌ {len(self.regressions)} regression(s) detected")
            
            # Group by severity
            severity_counts = {}
            for regression in self.regressions:
                severity = regression['severity']
                severity_counts[severity] = severity_counts.get(severity, 0) + 1
                
            for severity, count in severity_counts.items():
                icon = {"critical": "🔴", "major": "🟠", "moderate": "🟡", "minor": "🟢"}.get(severity, "⚪")
                print(f"   {icon} {severity.capitalize()}: {count}")
                
        else:
            print("✅ No significant regressions detected")
            
        print(f"🎯 Regression threshold: {self.threshold:.1%}")

def main():
    parser = argparse.ArgumentParser(description="Check for performance regressions")
    parser.add_argument("--analysis", required=True, help="Performance analysis JSON file")
    parser.add_argument("--threshold", type=float, default=0.05, help="Regression threshold (default: 5%)")
    parser.add_argument("--fail-on-regression", type=bool, default=True, help="Exit with error on regression")
    parser.add_argument("--output", help="Output regression report file")
    
    args = parser.parse_args()
    
    # Load analysis data
    analysis_path = Path(args.analysis)
    if not analysis_path.exists():
        print(f"❌ Analysis file not found: {analysis_path}")
        sys.exit(1)
        
    try:
        with open(analysis_path, 'r') as f:
            analysis_data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"❌ Error parsing analysis file: {e}")
        sys.exit(1)
        
    # Check for regressions
    checker = PerformanceRegressionChecker(args.threshold, args.fail_on_regression)
    no_regressions = checker.check_regressions(analysis_data)
    
    # Save regression report if requested
    if args.output:
        report_data = {
            'timestamp': analysis_data.get('timestamp', 'unknown'),
            'threshold': args.threshold,
            'regressions_detected': len(checker.regressions),
            'regressions': checker.regressions
        }
        
        with open(args.output, 'w') as f:
            json.dump(report_data, f, indent=2)
            
        print(f"📝 Regression report saved to: {args.output}")
    
    # Exit with appropriate code
    if args.fail_on_regression and not no_regressions:
        print(f"\n❌ Failing due to {len(checker.regressions)} regression(s)")
        sys.exit(1)
    else:
        print("\n✅ Performance regression check completed")
        sys.exit(0)

if __name__ == "__main__":
    main()
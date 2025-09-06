#!/usr/bin/env python3
"""
ECScope Performance Analysis Script

This script analyzes performance benchmark results from the comprehensive test suite,
compares them against baseline results, and generates detailed performance reports
with educational insights and optimization recommendations.

Features:
- Performance regression detection with statistical analysis
- Cross-platform performance comparison
- Memory usage analysis and optimization suggestions
- Educational insights about performance characteristics
- Automated report generation in multiple formats

Usage:
    python3 analyze_performance_results.py \
        --current ecs_benchmarks.json memory_benchmarks.json \
        --baseline performance_baseline.json \
        --output performance_analysis.json \
        --report performance_report.md \
        --threshold 0.05
"""

import json
import argparse
import statistics
import os
from datetime import datetime
from typing import Dict, List, Any, Optional, Tuple
from dataclasses import dataclass, asdict
from pathlib import Path

@dataclass
class PerformanceResult:
    """Individual performance benchmark result"""
    test_name: str
    category: str
    architecture: str
    entity_count: int
    average_time_us: float
    min_time_us: float
    max_time_us: float
    std_deviation_us: float
    entities_per_second: float
    memory_usage: int
    cache_hit_ratio: float
    timestamp: str
    platform: str = "unknown"
    build_config: str = "unknown"

@dataclass
class RegressionResult:
    """Performance regression analysis result"""
    test_name: str
    baseline_performance: float
    current_performance: float
    change_percentage: float
    is_regression: bool
    is_improvement: bool
    statistical_significance: float
    confidence_level: float

@dataclass
class PerformanceAnalysis:
    """Complete performance analysis results"""
    summary: Dict[str, Any]
    regressions: List[RegressionResult]
    improvements: List[RegressionResult]
    cross_platform_analysis: Dict[str, Any]
    memory_analysis: Dict[str, Any]
    educational_insights: List[str]
    optimization_recommendations: List[str]
    timestamp: str

class PerformanceAnalyzer:
    """Main performance analysis engine"""
    
    def __init__(self, regression_threshold: float = 0.05):
        self.regression_threshold = regression_threshold
        self.baseline_results: List[PerformanceResult] = []
        self.current_results: List[PerformanceResult] = []
        
    def load_results(self, file_path: str) -> List[PerformanceResult]:
        """Load performance results from JSON file"""
        try:
            with open(file_path, 'r') as f:
                data = json.load(f)
            
            results = []
            
            # Handle different JSON formats
            if isinstance(data, list):
                benchmark_data = data
            elif 'benchmarks' in data:
                benchmark_data = data['benchmarks']
            elif 'results' in data:
                benchmark_data = data['results']
            else:
                benchmark_data = [data]  # Single result
            
            for item in benchmark_data:
                try:
                    result = PerformanceResult(
                        test_name=item.get('name', item.get('test_name', 'Unknown')),
                        category=item.get('category', 'Unknown'),
                        architecture=item.get('architecture', item.get('architecture_type', 'Unknown')),
                        entity_count=int(item.get('entity_count', 0)),
                        average_time_us=float(item.get('real_time', item.get('average_time_us', 0))),
                        min_time_us=float(item.get('min_time', item.get('min_time_us', 0))),
                        max_time_us=float(item.get('max_time', item.get('max_time_us', 0))),
                        std_deviation_us=float(item.get('stddev', item.get('std_deviation_us', 0))),
                        entities_per_second=float(item.get('entities_per_second', 0)),
                        memory_usage=int(item.get('memory_usage', item.get('peak_memory_usage', 0))),
                        cache_hit_ratio=float(item.get('cache_hit_ratio', 0.0)),
                        timestamp=item.get('timestamp', datetime.now().isoformat()),
                        platform=item.get('platform', 'unknown'),
                        build_config=item.get('build_config', 'unknown')
                    )
                    results.append(result)
                except (KeyError, ValueError, TypeError) as e:
                    print(f"Warning: Failed to parse result item {item}: {e}")
                    continue
            
            return results
            
        except (FileNotFoundError, json.JSONDecodeError) as e:
            print(f"Error loading results from {file_path}: {e}")
            return []
    
    def detect_regressions(self, baseline: List[PerformanceResult], 
                          current: List[PerformanceResult]) -> Tuple[List[RegressionResult], List[RegressionResult]]:
        """Detect performance regressions and improvements"""
        regressions = []
        improvements = []
        
        # Create lookup dictionary for baseline results
        baseline_lookup = {}
        for result in baseline:
            key = f"{result.test_name}_{result.architecture}_{result.entity_count}"
            baseline_lookup[key] = result
        
        for current_result in current:
            key = f"{current_result.test_name}_{current_result.architecture}_{current_result.entity_count}"
            
            if key not in baseline_lookup:
                continue  # No baseline to compare against
            
            baseline_result = baseline_lookup[key]
            
            # Calculate performance change
            baseline_perf = baseline_result.average_time_us
            current_perf = current_result.average_time_us
            
            if baseline_perf == 0:
                continue  # Avoid division by zero
            
            change_percentage = (current_perf - baseline_perf) / baseline_perf
            
            # Calculate statistical significance (simplified)
            baseline_cv = baseline_result.std_deviation_us / baseline_perf if baseline_perf > 0 else 0
            current_cv = current_result.std_deviation_us / current_perf if current_perf > 0 else 0
            combined_variance = (baseline_cv ** 2 + current_cv ** 2) ** 0.5
            
            statistical_significance = abs(change_percentage) / (combined_variance + 1e-10)
            confidence_level = min(0.99, statistical_significance / 3.0)  # Rough approximation
            
            regression_result = RegressionResult(
                test_name=current_result.test_name,
                baseline_performance=baseline_perf,
                current_performance=current_perf,
                change_percentage=change_percentage,
                is_regression=change_percentage > self.regression_threshold,
                is_improvement=change_percentage < -self.regression_threshold,
                statistical_significance=statistical_significance,
                confidence_level=confidence_level
            )
            
            if regression_result.is_regression and confidence_level > 0.5:
                regressions.append(regression_result)
            elif regression_result.is_improvement and confidence_level > 0.5:
                improvements.append(regression_result)
        
        # Sort by magnitude of change
        regressions.sort(key=lambda x: x.change_percentage, reverse=True)
        improvements.sort(key=lambda x: x.change_percentage)
        
        return regressions, improvements
    
    def analyze_memory_performance(self, results: List[PerformanceResult]) -> Dict[str, Any]:
        """Analyze memory usage patterns and efficiency"""
        if not results:
            return {}
        
        memory_usage = [r.memory_usage for r in results if r.memory_usage > 0]
        if not memory_usage:
            return {"error": "No memory usage data available"}
        
        # Calculate memory statistics
        analysis = {
            "total_tests": len(results),
            "tests_with_memory_data": len(memory_usage),
            "min_memory_usage": min(memory_usage),
            "max_memory_usage": max(memory_usage),
            "average_memory_usage": statistics.mean(memory_usage),
            "median_memory_usage": statistics.median(memory_usage),
            "memory_usage_std": statistics.stdev(memory_usage) if len(memory_usage) > 1 else 0,
        }
        
        # Memory efficiency analysis
        memory_per_entity = []
        for result in results:
            if result.memory_usage > 0 and result.entity_count > 0:
                memory_per_entity.append(result.memory_usage / result.entity_count)
        
        if memory_per_entity:
            analysis["average_memory_per_entity"] = statistics.mean(memory_per_entity)
            analysis["memory_efficiency_variance"] = statistics.stdev(memory_per_entity) if len(memory_per_entity) > 1 else 0
        
        # Cache performance analysis
        cache_ratios = [r.cache_hit_ratio for r in results if r.cache_hit_ratio > 0]
        if cache_ratios:
            analysis["average_cache_hit_ratio"] = statistics.mean(cache_ratios)
            analysis["cache_performance_consistency"] = 1.0 - (statistics.stdev(cache_ratios) if len(cache_ratios) > 1 else 0)
        
        return analysis
    
    def generate_educational_insights(self, results: List[PerformanceResult], 
                                    regressions: List[RegressionResult],
                                    improvements: List[RegressionResult]) -> List[str]:
        """Generate educational insights about performance characteristics"""
        insights = []
        
        if not results:
            return ["No performance data available for analysis."]
        
        # Analyze architecture performance differences
        architectures = {}
        for result in results:
            if result.architecture not in architectures:
                architectures[result.architecture] = []
            architectures[result.architecture].append(result.average_time_us)
        
        if len(architectures) > 1:
            arch_averages = {arch: statistics.mean(times) for arch, times in architectures.items()}
            best_arch = min(arch_averages, key=arch_averages.get)
            worst_arch = max(arch_averages, key=arch_averages.get)
            
            improvement_ratio = arch_averages[worst_arch] / arch_averages[best_arch]
            
            insights.append(
                f"ECS Architecture Analysis: {best_arch} architecture performs {improvement_ratio:.2f}x "
                f"better than {worst_arch} on average. This demonstrates the importance of "
                f"data structure choice in high-performance systems."
            )
        
        # Analyze scaling characteristics
        entity_counts = sorted(set(r.entity_count for r in results if r.entity_count > 0))
        if len(entity_counts) > 2:
            scaling_data = []
            for count in entity_counts:
                count_results = [r for r in results if r.entity_count == count]
                if count_results:
                    avg_time = statistics.mean(r.average_time_us for r in count_results)
                    scaling_data.append((count, avg_time))
            
            if len(scaling_data) > 2:
                # Simple linear regression for scaling analysis
                n = len(scaling_data)
                sum_x = sum(x for x, y in scaling_data)
                sum_y = sum(y for x, y in scaling_data)
                sum_xy = sum(x * y for x, y in scaling_data)
                sum_x2 = sum(x * x for x, y in scaling_data)
                
                slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x)
                
                if slope > 0:
                    complexity_class = "linear" if slope < 2 else "quadratic" if slope < 10 else "exponential"
                    insights.append(
                        f"Scaling Analysis: Performance appears to scale {complexity_class}ly with entity count "
                        f"(slope: {slope:.3f}). This suggests the ECS implementation maintains good "
                        f"cache locality and avoids algorithmic bottlenecks."
                    )
        
        # Memory usage insights
        memory_results = [r for r in results if r.memory_usage > 0]
        if memory_results:
            avg_memory = statistics.mean(r.memory_usage for r in memory_results)
            max_memory = max(r.memory_usage for r in memory_results)
            
            if max_memory > avg_memory * 2:
                insights.append(
                    f"Memory Usage Pattern: Peak memory usage ({max_memory:,} bytes) is significantly "
                    f"higher than average ({avg_memory:,.0f} bytes), indicating potential memory "
                    f"fragmentation or inefficient allocation patterns."
                )
        
        # Regression insights
        if regressions:
            worst_regression = max(regressions, key=lambda x: x.change_percentage)
            insights.append(
                f"Performance Regression Alert: {worst_regression.test_name} shows a "
                f"{worst_regression.change_percentage * 100:.1f}% performance degradation. "
                f"This could indicate algorithmic changes, cache misses, or memory layout issues."
            )
        
        # Improvement insights  
        if improvements:
            best_improvement = min(improvements, key=lambda x: x.change_percentage)
            insights.append(
                f"Performance Improvement: {best_improvement.test_name} shows a "
                f"{abs(best_improvement.change_percentage) * 100:.1f}% performance improvement. "
                f"This demonstrates the impact of optimizations in high-performance computing."
            )
        
        return insights
    
    def generate_optimization_recommendations(self, results: List[PerformanceResult],
                                           regressions: List[RegressionResult],
                                           memory_analysis: Dict[str, Any]) -> List[str]:
        """Generate specific optimization recommendations"""
        recommendations = []
        
        # Memory-based recommendations
        if "average_memory_per_entity" in memory_analysis:
            memory_per_entity = memory_analysis["average_memory_per_entity"]
            if memory_per_entity > 1000:  # More than 1KB per entity
                recommendations.append(
                    "High Memory Usage: Consider using Structure of Arrays (SoA) data layout "
                    "instead of Array of Structures (AoS) to improve cache locality and reduce "
                    "memory overhead per entity."
                )
        
        if "cache_performance_consistency" in memory_analysis:
            cache_consistency = memory_analysis["cache_performance_consistency"]
            if cache_consistency < 0.8:  # Less than 80% consistency
                recommendations.append(
                    "Cache Performance Variance: High variance in cache hit ratios suggests "
                    "inconsistent memory access patterns. Consider implementing entity batching "
                    "or improving data locality through better component organization."
                )
        
        # Regression-based recommendations
        if regressions:
            regression_categories = {}
            for reg in regressions:
                # Extract category from test name (simplified)
                category = "ECS" if "ecs" in reg.test_name.lower() else \
                          "Memory" if "memory" in reg.test_name.lower() else \
                          "Physics" if "physics" in reg.test_name.lower() else "Other"
                
                if category not in regression_categories:
                    regression_categories[category] = []
                regression_categories[category].append(reg)
            
            for category, regs in regression_categories.items():
                if len(regs) > 1:
                    avg_regression = statistics.mean(r.change_percentage for r in regs)
                    recommendations.append(
                        f"{category} System Optimization: Multiple regressions detected "
                        f"(avg: {avg_regression * 100:.1f}%). Review recent changes to "
                        f"{category.lower()} system implementation, particularly algorithm "
                        f"complexity and data structure modifications."
                    )
        
        # Architecture-specific recommendations
        arch_performance = {}
        for result in results:
            if result.architecture not in arch_performance:
                arch_performance[result.architecture] = []
            arch_performance[result.architecture].append(result.average_time_us)
        
        if len(arch_performance) > 1:
            arch_averages = {arch: statistics.mean(times) for arch, times in arch_performance.items()}
            best_arch = min(arch_averages, key=arch_averages.get)
            
            recommendations.append(
                f"Architecture Optimization: {best_arch} architecture shows best performance "
                f"characteristics. Consider optimizing other architectures using similar "
                f"data layout and access patterns, or prioritize {best_arch} for "
                f"performance-critical applications."
            )
        
        # Entity scaling recommendations
        entity_counts = sorted(set(r.entity_count for r in results if r.entity_count > 0))
        if len(entity_counts) > 2:
            large_scale_results = [r for r in results if r.entity_count >= max(entity_counts) * 0.8]
            if large_scale_results:
                avg_large_scale_time = statistics.mean(r.average_time_us for r in large_scale_results)
                small_scale_results = [r for r in results if r.entity_count <= min(entity_counts) * 1.2]
                avg_small_scale_time = statistics.mean(r.average_time_us for r in small_scale_results)
                
                scaling_factor = avg_large_scale_time / avg_small_scale_time
                max_entities = max(entity_counts)
                min_entities = min(entity_counts)
                
                if scaling_factor > (max_entities / min_entities):
                    recommendations.append(
                        f"Scaling Optimization: Performance degrades more than linearly with "
                        f"entity count (factor: {scaling_factor:.2f}). Consider implementing "
                        f"spatial partitioning, level-of-detail systems, or parallel processing "
                        f"for large-scale scenarios."
                    )
        
        if not recommendations:
            recommendations.append(
                "Performance Baseline: Current performance characteristics appear optimal "
                "for the given workload. Continue monitoring for regressions and consider "
                "profiling with production-like data for further optimization opportunities."
            )
        
        return recommendations
    
    def run_analysis(self, current_files: List[str], baseline_file: Optional[str] = None) -> PerformanceAnalysis:
        """Run complete performance analysis"""
        # Load current results
        self.current_results = []
        for file_path in current_files:
            results = self.load_results(file_path)
            self.current_results.extend(results)
        
        if not self.current_results:
            raise ValueError("No current performance results found")
        
        # Load baseline results if available
        regressions = []
        improvements = []
        if baseline_file and os.path.exists(baseline_file):
            self.baseline_results = self.load_results(baseline_file)
            if self.baseline_results:
                regressions, improvements = self.detect_regressions(self.baseline_results, self.current_results)
        
        # Generate analysis components
        memory_analysis = self.analyze_memory_performance(self.current_results)
        educational_insights = self.generate_educational_insights(self.current_results, regressions, improvements)
        optimization_recommendations = self.generate_optimization_recommendations(
            self.current_results, regressions, memory_analysis)
        
        # Generate summary statistics
        summary = {
            "total_tests": len(self.current_results),
            "test_categories": len(set(r.category for r in self.current_results)),
            "architectures_tested": len(set(r.architecture for r in self.current_results)),
            "entity_counts_tested": sorted(set(r.entity_count for r in self.current_results if r.entity_count > 0)),
            "average_performance_us": statistics.mean(r.average_time_us for r in self.current_results),
            "performance_variance": statistics.stdev(r.average_time_us for r in self.current_results) if len(self.current_results) > 1 else 0,
            "regressions_detected": len(regressions),
            "improvements_detected": len(improvements),
            "has_baseline": baseline_file is not None and len(self.baseline_results) > 0
        }
        
        # Cross-platform analysis (simplified)
        platforms = set(r.platform for r in self.current_results if r.platform != "unknown")
        cross_platform_analysis = {
            "platforms_tested": list(platforms),
            "platform_count": len(platforms)
        }
        
        return PerformanceAnalysis(
            summary=summary,
            regressions=regressions,
            improvements=improvements,
            cross_platform_analysis=cross_platform_analysis,
            memory_analysis=memory_analysis,
            educational_insights=educational_insights,
            optimization_recommendations=optimization_recommendations,
            timestamp=datetime.now().isoformat()
        )
    
    def export_analysis(self, analysis: PerformanceAnalysis, output_file: str):
        """Export analysis results to JSON"""
        with open(output_file, 'w') as f:
            json.dump(asdict(analysis), f, indent=2, default=str)
        print(f"Analysis exported to {output_file}")
    
    def generate_report(self, analysis: PerformanceAnalysis, report_file: str):
        """Generate markdown performance report"""
        with open(report_file, 'w') as f:
            f.write("# ECScope Performance Analysis Report\n\n")
            f.write(f"Generated: {analysis.timestamp}\n\n")
            
            # Executive Summary
            f.write("## Executive Summary\n\n")
            f.write(f"- **Total Tests Analyzed**: {analysis.summary['total_tests']}\n")
            f.write(f"- **Test Categories**: {analysis.summary['test_categories']}\n")
            f.write(f"- **Architectures Tested**: {analysis.summary['architectures_tested']}\n")
            f.write(f"- **Performance Regressions**: {analysis.summary['regressions_detected']}\n")
            f.write(f"- **Performance Improvements**: {analysis.summary['improvements_detected']}\n")
            f.write(f"- **Average Performance**: {analysis.summary['average_performance_us']:.2f} μs\n\n")
            
            # Regressions
            if analysis.regressions:
                f.write("## Performance Regressions\n\n")
                f.write("| Test Name | Baseline (μs) | Current (μs) | Change (%) | Confidence |\n")
                f.write("|-----------|---------------|--------------|------------|------------|\n")
                for reg in analysis.regressions[:10]:  # Top 10
                    f.write(f"| {reg.test_name} | {reg.baseline_performance:.2f} | {reg.current_performance:.2f} | "
                           f"+{reg.change_percentage*100:.1f}% | {reg.confidence_level*100:.0f}% |\n")
                f.write("\n")
            
            # Improvements
            if analysis.improvements:
                f.write("## Performance Improvements\n\n")
                f.write("| Test Name | Baseline (μs) | Current (μs) | Change (%) | Confidence |\n")
                f.write("|-----------|---------------|--------------|------------|------------|\n")
                for imp in analysis.improvements[:10]:  # Top 10
                    f.write(f"| {imp.test_name} | {imp.baseline_performance:.2f} | {imp.current_performance:.2f} | "
                           f"{imp.change_percentage*100:.1f}% | {imp.confidence_level*100:.0f}% |\n")
                f.write("\n")
            
            # Memory Analysis
            if analysis.memory_analysis:
                f.write("## Memory Analysis\n\n")
                mem = analysis.memory_analysis
                if "average_memory_usage" in mem:
                    f.write(f"- **Average Memory Usage**: {mem['average_memory_usage']:,.0f} bytes\n")
                    f.write(f"- **Peak Memory Usage**: {mem['max_memory_usage']:,} bytes\n")
                if "average_memory_per_entity" in mem:
                    f.write(f"- **Memory per Entity**: {mem['average_memory_per_entity']:.2f} bytes\n")
                if "average_cache_hit_ratio" in mem:
                    f.write(f"- **Cache Hit Ratio**: {mem['average_cache_hit_ratio']*100:.1f}%\n")
                f.write("\n")
            
            # Educational Insights
            f.write("## Educational Insights\n\n")
            for i, insight in enumerate(analysis.educational_insights, 1):
                f.write(f"{i}. {insight}\n\n")
            
            # Optimization Recommendations
            f.write("## Optimization Recommendations\n\n")
            for i, rec in enumerate(analysis.optimization_recommendations, 1):
                f.write(f"{i}. {rec}\n\n")
            
            # Technical Details
            f.write("## Technical Details\n\n")
            if analysis.summary["entity_counts_tested"]:
                entity_counts = analysis.summary["entity_counts_tested"]
                f.write(f"- **Entity Counts Tested**: {', '.join(map(str, entity_counts))}\n")
            f.write(f"- **Performance Variance**: {analysis.summary['performance_variance']:.2f} μs\n")
            if analysis.cross_platform_analysis["platforms_tested"]:
                platforms = ", ".join(analysis.cross_platform_analysis["platforms_tested"])
                f.write(f"- **Platforms**: {platforms}\n")
        
        print(f"Report generated: {report_file}")

def main():
    parser = argparse.ArgumentParser(description='Analyze ECScope performance benchmark results')
    parser.add_argument('--current', nargs='+', required=True, help='Current benchmark result files')
    parser.add_argument('--baseline', help='Baseline benchmark result file')
    parser.add_argument('--output', required=True, help='Output analysis file (JSON)')
    parser.add_argument('--report', help='Output report file (Markdown)')
    parser.add_argument('--threshold', type=float, default=0.05, help='Regression threshold (default: 0.05)')
    
    args = parser.parse_args()
    
    try:
        # Initialize analyzer
        analyzer = PerformanceAnalyzer(regression_threshold=args.threshold)
        
        # Run analysis
        print("Running performance analysis...")
        analysis = analyzer.run_analysis(args.current, args.baseline)
        
        # Export results
        analyzer.export_analysis(analysis, args.output)
        
        if args.report:
            analyzer.generate_report(analysis, args.report)
        
        # Print summary
        print("\nAnalysis Summary:")
        print(f"  Tests analyzed: {analysis.summary['total_tests']}")
        print(f"  Regressions detected: {analysis.summary['regressions_detected']}")
        print(f"  Improvements detected: {analysis.summary['improvements_detected']}")
        print(f"  Educational insights: {len(analysis.educational_insights)}")
        print(f"  Optimization recommendations: {len(analysis.optimization_recommendations)}")
        
        # Exit with appropriate code
        if analysis.summary['regressions_detected'] > 0:
            print(f"\nWarning: {analysis.summary['regressions_detected']} performance regressions detected")
            exit(1)
        else:
            print("\nNo significant performance regressions detected")
            exit(0)
            
    except Exception as e:
        print(f"Error during analysis: {e}")
        exit(2)

if __name__ == "__main__":
    main()
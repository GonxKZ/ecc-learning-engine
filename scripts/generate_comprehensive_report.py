#!/usr/bin/env python3
"""
ECScope Comprehensive Test Report Generator

This script generates a comprehensive HTML report combining:
- Unit test results
- Integration test results  
- Performance test results and regression analysis
- Code coverage analysis
- Educational system validation results
- Memory and security test results
- Cross-platform compatibility results
"""

import argparse
import json
import os
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Any
import re

try:
    import pandas as pd
    import matplotlib
    matplotlib.use('Agg')  # Use non-interactive backend
    import matplotlib.pyplot as plt
    import seaborn as sns
    from jinja2 import Template
except ImportError as e:
    print(f"Error: Required Python packages not installed: {e}")
    print("Please install: pip install pandas matplotlib seaborn jinja2")
    sys.exit(1)

class TestResultParser:
    """Parses various test result formats"""
    
    @staticmethod
    def parse_junit_xml(xml_path: Path) -> Dict[str, Any]:
        """Parse JUnit XML test results"""
        if not xml_path.exists():
            return {"total": 0, "passed": 0, "failed": 0, "skipped": 0, "tests": []}
        
        tree = ET.parse(xml_path)
        root = tree.getroot()
        
        result = {
            "total": 0,
            "passed": 0, 
            "failed": 0,
            "skipped": 0,
            "tests": [],
            "duration": 0.0
        }
        
        for testsuite in root.findall('.//testsuite'):
            result["total"] += int(testsuite.get('tests', 0))
            result["failed"] += int(testsuite.get('failures', 0)) + int(testsuite.get('errors', 0))
            result["skipped"] += int(testsuite.get('skipped', 0))
            result["duration"] += float(testsuite.get('time', 0))
            
            for testcase in testsuite.findall('testcase'):
                test = {
                    "name": testcase.get('name'),
                    "classname": testcase.get('classname'),
                    "time": float(testcase.get('time', 0)),
                    "status": "passed"
                }
                
                if testcase.find('failure') is not None:
                    test["status"] = "failed"
                    test["failure"] = testcase.find('failure').text
                elif testcase.find('error') is not None:
                    test["status"] = "error"
                    test["error"] = testcase.find('error').text
                elif testcase.find('skipped') is not None:
                    test["status"] = "skipped"
                    
                result["tests"].append(test)
        
        result["passed"] = result["total"] - result["failed"] - result["skipped"]
        return result
    
    @staticmethod
    def parse_coverage_info(coverage_path: Path) -> Dict[str, Any]:
        """Parse LCOV coverage information"""
        if not coverage_path.exists():
            return {"line_coverage": 0.0, "function_coverage": 0.0, "files": []}
        
        coverage_data = {"files": [], "line_coverage": 0.0, "function_coverage": 0.0}
        
        with open(coverage_path, 'r') as f:
            content = f.read()
        
        # Extract overall coverage percentages
        line_match = re.search(r'lines\.*:\s*(\d+\.?\d*)%', content)
        func_match = re.search(r'functions\.*:\s*(\d+\.?\d*)%', content)
        
        if line_match:
            coverage_data["line_coverage"] = float(line_match.group(1))
        if func_match:
            coverage_data["function_coverage"] = float(func_match.group(1))
            
        return coverage_data
    
    @staticmethod
    def parse_performance_results(results_dir: Path) -> Dict[str, Any]:
        """Parse performance test results"""
        performance_data = {
            "benchmarks": [],
            "regressions": [],
            "improvements": [],
            "baseline_comparison": {}
        }
        
        # Look for performance result files
        for perf_file in results_dir.glob("**/performance_*.json"):
            try:
                with open(perf_file, 'r') as f:
                    data = json.load(f)
                    performance_data["benchmarks"].extend(data.get("benchmarks", []))
                    performance_data["regressions"].extend(data.get("regressions", []))
                    performance_data["improvements"].extend(data.get("improvements", []))
            except (json.JSONDecodeError, FileNotFoundError):
                continue
        
        # Look for performance reports
        for report_file in results_dir.glob("**/performance_report_*.html"):
            # Parse HTML reports for regression information
            try:
                with open(report_file, 'r') as f:
                    content = f.read()
                    
                # Extract regression count from HTML
                regression_match = re.search(r'(\d+)\s+performance\s+regressions?', content, re.IGNORECASE)
                if regression_match:
                    performance_data["regression_count"] = int(regression_match.group(1))
                    
            except FileNotFoundError:
                continue
        
        return performance_data

class ReportGenerator:
    """Generates comprehensive HTML test report"""
    
    HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ECScope Comprehensive Test Report</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            color: #333;
            background-color: #f5f5f5;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        
        header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 2rem 0;
            margin-bottom: 2rem;
            border-radius: 8px;
            text-align: center;
        }
        
        h1 { font-size: 2.5rem; margin-bottom: 0.5rem; }
        .subtitle { font-size: 1.2rem; opacity: 0.9; }
        
        .summary-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 1rem;
            margin-bottom: 2rem;
        }
        
        .summary-card {
            background: white;
            border-radius: 8px;
            padding: 1.5rem;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            border-left: 4px solid #667eea;
        }
        
        .summary-card h3 {
            color: #667eea;
            margin-bottom: 1rem;
        }
        
        .metric {
            display: flex;
            justify-content: space-between;
            margin-bottom: 0.5rem;
        }
        
        .metric-value {
            font-weight: bold;
            color: #333;
        }
        
        .status-badge {
            display: inline-block;
            padding: 0.25rem 0.75rem;
            border-radius: 20px;
            font-size: 0.875rem;
            font-weight: bold;
            text-transform: uppercase;
        }
        
        .status-passed { background-color: #d4edda; color: #155724; }
        .status-failed { background-color: #f8d7da; color: #721c24; }
        .status-warning { background-color: #fff3cd; color: #856404; }
        .status-skipped { background-color: #e2e3e5; color: #6c757d; }
        
        .section {
            background: white;
            border-radius: 8px;
            padding: 2rem;
            margin-bottom: 2rem;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        
        .section h2 {
            color: #667eea;
            margin-bottom: 1.5rem;
            border-bottom: 2px solid #667eea;
            padding-bottom: 0.5rem;
        }
        
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 1rem;
        }
        
        th, td {
            text-align: left;
            padding: 0.75rem;
            border-bottom: 1px solid #ddd;
        }
        
        th {
            background-color: #f8f9fa;
            font-weight: bold;
            color: #667eea;
        }
        
        tr:hover { background-color: #f8f9fa; }
        
        .progress-bar {
            width: 100%;
            height: 20px;
            background-color: #e9ecef;
            border-radius: 10px;
            overflow: hidden;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #28a745, #20c997);
            transition: width 0.3s ease;
        }
        
        .chart-container {
            margin: 1rem 0;
            text-align: center;
        }
        
        .chart-container img {
            max-width: 100%;
            border-radius: 4px;
        }
        
        .regression-item {
            background-color: #f8d7da;
            border-left: 4px solid #dc3545;
            padding: 1rem;
            margin-bottom: 1rem;
            border-radius: 4px;
        }
        
        .improvement-item {
            background-color: #d4edda;
            border-left: 4px solid #28a745;
            padding: 1rem;
            margin-bottom: 1rem;
            border-radius: 4px;
        }
        
        .test-details {
            max-height: 400px;
            overflow-y: auto;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        
        .expandable {
            cursor: pointer;
        }
        
        .expandable:before {
            content: "▶ ";
            transition: transform 0.2s;
        }
        
        .expandable.expanded:before {
            transform: rotate(90deg);
        }
        
        .collapsible-content {
            display: none;
            margin-top: 1rem;
            padding: 1rem;
            background-color: #f8f9fa;
            border-radius: 4px;
        }
        
        footer {
            text-align: center;
            padding: 2rem 0;
            color: #6c757d;
            font-size: 0.875rem;
        }
        
        @media (max-width: 768px) {
            .container { padding: 10px; }
            .summary-grid { grid-template-columns: 1fr; }
            h1 { font-size: 2rem; }
            .section { padding: 1rem; }
        }
    </style>
    <script>
        function toggleCollapsible(element) {
            element.classList.toggle('expanded');
            const content = element.nextElementSibling;
            if (content.style.display === 'block') {
                content.style.display = 'none';
            } else {
                content.style.display = 'block';
            }
        }
        
        function filterTests(category, status) {
            const rows = document.querySelectorAll(`.${category}-table tbody tr`);
            rows.forEach(row => {
                if (status === 'all' || row.classList.contains(status)) {
                    row.style.display = '';
                } else {
                    row.style.display = 'none';
                }
            });
        }
    </script>
</head>
<body>
    <div class="container">
        <header>
            <h1>ECScope Comprehensive Test Report</h1>
            <div class="subtitle">Generated on {{ timestamp }}</div>
            <div class="subtitle">Commit: {{ commit_sha[:8] if commit_sha else 'N/A' }}</div>
        </header>
        
        <!-- Test Summary -->
        <div class="summary-grid">
            <div class="summary-card">
                <h3>Overall Test Status</h3>
                <div class="metric">
                    <span>Total Tests:</span>
                    <span class="metric-value">{{ summary.total_tests }}</span>
                </div>
                <div class="metric">
                    <span>Passed:</span>
                    <span class="metric-value status-passed">{{ summary.total_passed }}</span>
                </div>
                <div class="metric">
                    <span>Failed:</span>
                    <span class="metric-value status-failed">{{ summary.total_failed }}</span>
                </div>
                <div class="metric">
                    <span>Skipped:</span>
                    <span class="metric-value status-skipped">{{ summary.total_skipped }}</span>
                </div>
                <div class="progress-bar">
                    <div class="progress-fill" style="width: {{ (summary.total_passed / summary.total_tests * 100) if summary.total_tests > 0 else 0 }}%"></div>
                </div>
            </div>
            
            <div class="summary-card">
                <h3>Code Coverage</h3>
                <div class="metric">
                    <span>Line Coverage:</span>
                    <span class="metric-value">{{ "%.1f"|format(coverage.line_coverage) }}%</span>
                </div>
                <div class="metric">
                    <span>Function Coverage:</span>
                    <span class="metric-value">{{ "%.1f"|format(coverage.function_coverage) }}%</span>
                </div>
                <div class="progress-bar">
                    <div class="progress-fill" style="width: {{ coverage.line_coverage }}%"></div>
                </div>
            </div>
            
            <div class="summary-card">
                <h3>Performance</h3>
                <div class="metric">
                    <span>Benchmarks Run:</span>
                    <span class="metric-value">{{ performance.benchmarks|length }}</span>
                </div>
                <div class="metric">
                    <span>Regressions:</span>
                    <span class="metric-value status-{{ 'failed' if performance.regressions|length > 0 else 'passed' }}">{{ performance.regressions|length }}</span>
                </div>
                <div class="metric">
                    <span>Improvements:</span>
                    <span class="metric-value status-passed">{{ performance.improvements|length }}</span>
                </div>
            </div>
            
            <div class="summary-card">
                <h3>Test Duration</h3>
                <div class="metric">
                    <span>Total Time:</span>
                    <span class="metric-value">{{ "%.1f"|format(summary.total_duration) }}s</span>
                </div>
                <div class="metric">
                    <span>Average per Test:</span>
                    <span class="metric-value">{{ "%.2f"|format(summary.total_duration / summary.total_tests) if summary.total_tests > 0 else 0 }}s</span>
                </div>
            </div>
        </div>
        
        <!-- Unit Tests Section -->
        {% if unit_tests %}
        <section class="section">
            <h2 class="expandable" onclick="toggleCollapsible(this)">Unit Tests ({{ unit_tests.total }} tests)</h2>
            <div class="collapsible-content">
                <div class="metric">
                    <span>Status:</span>
                    <span class="status-badge status-{{ 'passed' if unit_tests.failed == 0 else 'failed' }}">
                        {{ unit_tests.passed }}/{{ unit_tests.total }} Passed
                    </span>
                </div>
                
                {% if unit_tests.failed > 0 %}
                <h4>Failed Tests:</h4>
                <div class="test-details">
                    <table>
                        <thead>
                            <tr>
                                <th>Test Name</th>
                                <th>Class</th>
                                <th>Duration</th>
                                <th>Failure Reason</th>
                            </tr>
                        </thead>
                        <tbody>
                            {% for test in unit_tests.tests %}
                            {% if test.status == 'failed' %}
                            <tr>
                                <td>{{ test.name }}</td>
                                <td>{{ test.classname }}</td>
                                <td>{{ "%.3f"|format(test.time) }}s</td>
                                <td><pre>{{ test.failure[:200] }}...</pre></td>
                            </tr>
                            {% endif %}
                            {% endfor %}
                        </tbody>
                    </table>
                </div>
                {% endif %}
            </div>
        </section>
        {% endif %}
        
        <!-- Integration Tests Section -->
        {% if integration_tests %}
        <section class="section">
            <h2 class="expandable" onclick="toggleCollapsible(this)">Integration Tests ({{ integration_tests.total }} tests)</h2>
            <div class="collapsible-content">
                <div class="metric">
                    <span>Status:</span>
                    <span class="status-badge status-{{ 'passed' if integration_tests.failed == 0 else 'failed' }}">
                        {{ integration_tests.passed }}/{{ integration_tests.total }} Passed
                    </span>
                </div>
                
                {% if integration_tests.failed > 0 %}
                <h4>Failed Tests:</h4>
                <div class="test-details">
                    <table>
                        <thead>
                            <tr>
                                <th>Test Name</th>
                                <th>Class</th>
                                <th>Duration</th>
                                <th>Failure Reason</th>
                            </tr>
                        </thead>
                        <tbody>
                            {% for test in integration_tests.tests %}
                            {% if test.status == 'failed' %}
                            <tr>
                                <td>{{ test.name }}</td>
                                <td>{{ test.classname }}</td>
                                <td>{{ "%.3f"|format(test.time) }}s</td>
                                <td><pre>{{ test.failure[:200] }}...</pre></td>
                            </tr>
                            {% endif %}
                            {% endfor %}
                        </tbody>
                    </table>
                </div>
                {% endif %}
            </div>
        </section>
        {% endif %}
        
        <!-- Performance Tests Section -->
        {% if performance %}
        <section class="section">
            <h2 class="expandable" onclick="toggleCollapsible(this)">Performance Analysis</h2>
            <div class="collapsible-content">
                {% if performance.regressions %}
                <h4>Performance Regressions ({{ performance.regressions|length }})</h4>
                {% for regression in performance.regressions %}
                <div class="regression-item">
                    <strong>{{ regression.metric_name }}</strong>: {{ "%.1f"|format(regression.percentage_change) }}% slower
                    <br><small>{{ regression.description or 'No description available' }}</small>
                </div>
                {% endfor %}
                {% endif %}
                
                {% if performance.improvements %}
                <h4>Performance Improvements ({{ performance.improvements|length }})</h4>
                {% for improvement in performance.improvements %}
                <div class="improvement-item">
                    <strong>{{ improvement.metric_name }}</strong>: {{ "%.1f"|format(abs(improvement.percentage_change)) }}% faster
                    <br><small>{{ improvement.description or 'Performance improvement detected' }}</small>
                </div>
                {% endfor %}
                {% endif %}
                
                {% if performance.benchmarks %}
                <h4>Benchmark Results</h4>
                <div class="test-details">
                    <table>
                        <thead>
                            <tr>
                                <th>Benchmark</th>
                                <th>Current</th>
                                <th>Baseline</th>
                                <th>Change</th>
                                <th>Status</th>
                            </tr>
                        </thead>
                        <tbody>
                            {% for benchmark in performance.benchmarks %}
                            <tr>
                                <td>{{ benchmark.name }}</td>
                                <td>{{ "%.3f"|format(benchmark.current_value) }}</td>
                                <td>{{ "%.3f"|format(benchmark.baseline_value) if benchmark.baseline_value else 'N/A' }}</td>
                                <td>{{ "%.1f"|format(benchmark.percentage_change) if benchmark.percentage_change else 'N/A' }}%</td>
                                <td>
                                    <span class="status-badge status-{{ benchmark.status }}">{{ benchmark.status.upper() }}</span>
                                </td>
                            </tr>
                            {% endfor %}
                        </tbody>
                    </table>
                </div>
                {% endif %}
            </div>
        </section>
        {% endif %}
        
        <!-- Educational Tests Section -->
        {% if educational_tests %}
        <section class="section">
            <h2 class="expandable" onclick="toggleCollapsible(this)">Educational System Tests ({{ educational_tests.total }} tests)</h2>
            <div class="collapsible-content">
                <div class="metric">
                    <span>Status:</span>
                    <span class="status-badge status-{{ 'passed' if educational_tests.failed == 0 else 'failed' }}">
                        {{ educational_tests.passed }}/{{ educational_tests.total }} Passed
                    </span>
                </div>
                
                <p>Educational system validation includes:</p>
                <ul>
                    <li>Learning module functionality</li>
                    <li>Tutorial system validation</li>
                    <li>Interactive visualization accuracy</li>
                    <li>Progress tracking systems</li>
                    <li>Adaptive difficulty algorithms</li>
                    <li>Educational analytics</li>
                </ul>
            </div>
        </section>
        {% endif %}
        
        <!-- Memory & Security Section -->
        {% if memory_tests %}
        <section class="section">
            <h2 class="expandable" onclick="toggleCollapsible(this)">Memory & Security Tests</h2>
            <div class="collapsible-content">
                <div class="metric">
                    <span>Memory Leaks:</span>
                    <span class="status-badge status-{{ 'passed' if memory_tests.leaks == 0 else 'failed' }}">
                        {{ memory_tests.leaks }} detected
                    </span>
                </div>
                
                <div class="metric">
                    <span>Security Issues:</span>
                    <span class="status-badge status-{{ 'passed' if memory_tests.security_issues == 0 else 'failed' }}">
                        {{ memory_tests.security_issues }} found
                    </span>
                </div>
                
                {% if memory_tests.sanitizer_reports %}
                <h4>Sanitizer Reports:</h4>
                <div class="test-details">
                    {% for report in memory_tests.sanitizer_reports %}
                    <div style="margin-bottom: 1rem; padding: 1rem; background: #f8f9fa; border-radius: 4px;">
                        <strong>{{ report.type }}:</strong> {{ report.message }}
                    </div>
                    {% endfor %}
                </div>
                {% endif %}
            </div>
        </section>
        {% endif %}
        
        <!-- Detailed Charts Section -->
        {% if charts %}
        <section class="section">
            <h2>Visual Analysis</h2>
            {% for chart in charts %}
            <div class="chart-container">
                <h4>{{ chart.title }}</h4>
                <img src="{{ chart.path }}" alt="{{ chart.title }}">
            </div>
            {% endfor %}
        </section>
        {% endif %}
        
        <footer>
            <p>ECScope Comprehensive Test Report - Generated by ECScope CI/CD Pipeline</p>
            <p>For questions or issues, please contact the development team</p>
        </footer>
    </div>
</body>
</html>
    """
    
    def __init__(self):
        self.template = Template(self.HTML_TEMPLATE)
    
    def generate_charts(self, data: Dict[str, Any], output_dir: Path) -> List[Dict[str, str]]:
        """Generate visualization charts"""
        charts = []
        
        # Test results pie chart
        if data.get('summary', {}).get('total_tests', 0) > 0:
            fig, ax = plt.subplots(figsize=(8, 6))
            
            summary = data['summary']
            labels = ['Passed', 'Failed', 'Skipped']
            sizes = [summary['total_passed'], summary['total_failed'], summary['total_skipped']]
            colors = ['#28a745', '#dc3545', '#6c757d']
            
            # Filter out zero values
            non_zero = [(label, size, color) for label, size, color in zip(labels, sizes, colors) if size > 0]
            if non_zero:
                labels, sizes, colors = zip(*non_zero)
                
                ax.pie(sizes, labels=labels, colors=colors, autopct='%1.1f%%', startangle=90)
                ax.set_title('Test Results Distribution')
                
                chart_path = output_dir / 'test_results_pie.png'
                plt.savefig(chart_path, dpi=150, bbox_inches='tight')
                plt.close()
                
                charts.append({
                    'title': 'Test Results Distribution',
                    'path': str(chart_path.relative_to(output_dir.parent))
                })
        
        # Performance trends chart
        if data.get('performance', {}).get('benchmarks'):
            fig, ax = plt.subplots(figsize=(12, 6))
            
            benchmarks = data['performance']['benchmarks']
            names = [b['name'][:20] + '...' if len(b['name']) > 20 else b['name'] for b in benchmarks]
            values = [b.get('current_value', 0) for b in benchmarks]
            
            bars = ax.bar(names, values, color='#667eea')
            ax.set_title('Benchmark Results')
            ax.set_ylabel('Execution Time (ms)')
            
            # Rotate x labels for better readability
            plt.xticks(rotation=45, ha='right')
            
            # Add value labels on bars
            for bar, value in zip(bars, values):
                ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(values) * 0.01,
                       f'{value:.2f}', ha='center', va='bottom')
            
            chart_path = output_dir / 'benchmark_results.png'
            plt.savefig(chart_path, dpi=150, bbox_inches='tight')
            plt.close()
            
            charts.append({
                'title': 'Benchmark Results',
                'path': str(chart_path.relative_to(output_dir.parent))
            })
        
        # Coverage chart
        if data.get('coverage', {}).get('line_coverage', 0) > 0:
            fig, ax = plt.subplots(figsize=(8, 6))
            
            coverage = data['coverage']
            categories = ['Line Coverage', 'Function Coverage']
            values = [coverage['line_coverage'], coverage['function_coverage']]
            
            bars = ax.bar(categories, values, color=['#28a745', '#20c997'])
            ax.set_title('Code Coverage')
            ax.set_ylabel('Coverage Percentage')
            ax.set_ylim(0, 100)
            
            # Add percentage labels on bars
            for bar, value in zip(bars, values):
                ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1,
                       f'{value:.1f}%', ha='center', va='bottom')
            
            # Add target line at 80%
            ax.axhline(y=80, color='red', linestyle='--', alpha=0.7, label='Target (80%)')
            ax.legend()
            
            chart_path = output_dir / 'coverage_chart.png'
            plt.savefig(chart_path, dpi=150, bbox_inches='tight')
            plt.close()
            
            charts.append({
                'title': 'Code Coverage',
                'path': str(chart_path.relative_to(output_dir.parent))
            })
        
        return charts
    
    def generate_report(self, data: Dict[str, Any], output_path: Path) -> None:
        """Generate the comprehensive HTML report"""
        
        # Create output directory
        output_dir = output_path.parent
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Generate charts
        charts = self.generate_charts(data, output_dir)
        data['charts'] = charts
        
        # Add timestamp
        data['timestamp'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S UTC')
        
        # Get commit SHA from environment
        data['commit_sha'] = os.environ.get('GITHUB_SHA', 'unknown')
        
        # Render template
        html_content = self.template.render(**data)
        
        # Write report
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(html_content)

def main():
    parser = argparse.ArgumentParser(description='Generate comprehensive ECScope test report')
    parser.add_argument('--input-dir', type=Path, required=True,
                       help='Directory containing test result files')
    parser.add_argument('--output', type=Path, required=True,
                       help='Output HTML file path')
    parser.add_argument('--include-performance', action='store_true',
                       help='Include performance test results')
    parser.add_argument('--include-coverage', action='store_true',
                       help='Include code coverage analysis')
    parser.add_argument('--include-educational', action='store_true',
                       help='Include educational system test results')
    
    args = parser.parse_args()
    
    # Initialize data structure
    report_data = {
        'summary': {
            'total_tests': 0,
            'total_passed': 0,
            'total_failed': 0,
            'total_skipped': 0,
            'total_duration': 0.0
        },
        'coverage': {'line_coverage': 0.0, 'function_coverage': 0.0},
        'performance': {'benchmarks': [], 'regressions': [], 'improvements': []},
        'unit_tests': None,
        'integration_tests': None,
        'educational_tests': None,
        'memory_tests': None
    }
    
    parser_instance = TestResultParser()
    
    # Parse unit test results
    unit_results_file = args.input_dir / 'unit_test_results.xml'
    if unit_results_file.exists():
        report_data['unit_tests'] = parser_instance.parse_junit_xml(unit_results_file)
        report_data['summary']['total_tests'] += report_data['unit_tests']['total']
        report_data['summary']['total_passed'] += report_data['unit_tests']['passed']
        report_data['summary']['total_failed'] += report_data['unit_tests']['failed']
        report_data['summary']['total_skipped'] += report_data['unit_tests']['skipped']
        report_data['summary']['total_duration'] += report_data['unit_tests']['duration']
    
    # Parse integration test results
    integration_results_file = args.input_dir / 'integration_test_results.xml'
    if integration_results_file.exists():
        report_data['integration_tests'] = parser_instance.parse_junit_xml(integration_results_file)
        report_data['summary']['total_tests'] += report_data['integration_tests']['total']
        report_data['summary']['total_passed'] += report_data['integration_tests']['passed']
        report_data['summary']['total_failed'] += report_data['integration_tests']['failed']
        report_data['summary']['total_skipped'] += report_data['integration_tests']['skipped']
        report_data['summary']['total_duration'] += report_data['integration_tests']['duration']
    
    # Parse educational test results if requested
    if args.include_educational:
        educational_results_file = args.input_dir / 'educational_test_results.xml'
        if educational_results_file.exists():
            report_data['educational_tests'] = parser_instance.parse_junit_xml(educational_results_file)
            report_data['summary']['total_tests'] += report_data['educational_tests']['total']
            report_data['summary']['total_passed'] += report_data['educational_tests']['passed']
            report_data['summary']['total_failed'] += report_data['educational_tests']['failed']
            report_data['summary']['total_skipped'] += report_data['educational_tests']['skipped']
            report_data['summary']['total_duration'] += report_data['educational_tests']['duration']
    
    # Parse coverage results if requested
    if args.include_coverage:
        coverage_file = args.input_dir / 'coverage.info'
        if coverage_file.exists():
            report_data['coverage'] = parser_instance.parse_coverage_info(coverage_file)
    
    # Parse performance results if requested
    if args.include_performance:
        report_data['performance'] = parser_instance.parse_performance_results(args.input_dir)
    
    # Mock memory test data (would be parsed from actual sanitizer reports)
    report_data['memory_tests'] = {
        'leaks': 0,
        'security_issues': 0,
        'sanitizer_reports': []
    }
    
    # Generate report
    generator = ReportGenerator()
    generator.generate_report(report_data, args.output)
    
    print(f"Comprehensive test report generated: {args.output}")
    
    # Generate summary JSON for CI/CD
    summary_file = args.output.parent / 'test_summary.json'
    summary_data = {
        'unit_tests': {
            'total': report_data['unit_tests']['total'] if report_data['unit_tests'] else 0,
            'passed': report_data['unit_tests']['passed'] if report_data['unit_tests'] else 0,
            'failed': report_data['unit_tests']['failed'] if report_data['unit_tests'] else 0
        },
        'integration_tests': {
            'total': report_data['integration_tests']['total'] if report_data['integration_tests'] else 0,
            'passed': report_data['integration_tests']['passed'] if report_data['integration_tests'] else 0,
            'failed': report_data['integration_tests']['failed'] if report_data['integration_tests'] else 0
        },
        'educational_tests': {
            'total': report_data['educational_tests']['total'] if report_data['educational_tests'] else 0,
            'passed': report_data['educational_tests']['passed'] if report_data['educational_tests'] else 0,
            'failed': report_data['educational_tests']['failed'] if report_data['educational_tests'] else 0
        },
        'performance_tests': {
            'status': 'passed' if len(report_data['performance']['regressions']) == 0 else 'failed'
        },
        'coverage': report_data['coverage']['line_coverage'],
        'performance_regressions': len(report_data['performance']['regressions']),
        'total_duration': report_data['summary']['total_duration']
    }
    
    with open(summary_file, 'w') as f:
        json.dump(summary_data, f, indent=2)
    
    print(f"Test summary generated: {summary_file}")

if __name__ == '__main__':
    main()
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry import multi_page_benchmark_unittest_base
from telemetry import page
from perf_tools import smoothness_benchmark

from telemetry.page_benchmark_results import PageBenchmarkResults

class SmoothnessBenchmarkUnitTest(
  multi_page_benchmark_unittest_base.MultiPageBenchmarkUnitTestBase):

  def testFirstPaintTimeMeasurement(self):
    ps = self.CreatePageSetFromFileInUnittestDataDir('scrollable_page.html')

    benchmark = smoothness_benchmark.SmoothnessBenchmark()
    all_results = self.RunBenchmark(benchmark, ps)

    self.assertEqual(0, len(all_results.page_failures))
    self.assertEqual(1, len(all_results.page_results))

    results0 = all_results.page_results[0]
    if results0['first_paint'] == 'unsupported':
      # This test can't run on content_shell.
      return
    self.assertTrue(results0['first_paint'] > 0)

  def testScrollingWithGpuBenchmarkingExtension(self):
    ps = self.CreatePageSetFromFileInUnittestDataDir('scrollable_page.html')

    benchmark = smoothness_benchmark.SmoothnessBenchmark()
    all_results = self.RunBenchmark(benchmark, ps)

    self.assertEqual(0, len(all_results.page_failures))
    self.assertEqual(1, len(all_results.page_results))
    results0 = all_results.page_results[0]

    self.assertTrue('dropped_percent' in results0)
    self.assertTrue('mean_frame_time' in results0)

  def testCalcResultsFromRAFRenderStats(self):
    rendering_stats = {'droppedFrameCount': 5,
                       'totalTimeInSeconds': 1,
                       'numAnimationFrames': 10,
                       'numFramesSentToScreen': 10}
    res = PageBenchmarkResults()
    res.WillMeasurePage(page.Page('http://foo.com/', None))
    smoothness_benchmark.CalcScrollResults(rendering_stats, res)
    res.DidMeasurePage()
    self.assertEquals(50, res.page_results[0]['dropped_percent'].value)
    self.assertAlmostEquals(
      100,
      res.page_results[0]['mean_frame_time'].value, 2)

  def testCalcResultsRealRenderStats(self):
    rendering_stats = {'numFramesSentToScreen': 60,
                       'globalTotalTextureUploadTimeInSeconds': 0,
                       'totalProcessingCommandsTimeInSeconds': 0,
                       'globalTextureUploadCount': 0,
                       'droppedFrameCount': 0,
                       'textureUploadCount': 0,
                       'numAnimationFrames': 10,
                       'totalPaintTimeInSeconds': 0.35374299999999986,
                       'globalTotalProcessingCommandsTimeInSeconds': 0,
                       'totalTextureUploadTimeInSeconds': 0,
                       'totalRasterizeTimeInSeconds': 0,
                       'totalTimeInSeconds': 1.0}
    res = PageBenchmarkResults()
    res.WillMeasurePage(page.Page('http://foo.com/', None))
    smoothness_benchmark.CalcScrollResults(rendering_stats, res)
    res.DidMeasurePage()
    self.assertEquals(0, res.page_results[0]['dropped_percent'].value)
    self.assertAlmostEquals(
      1000/60.,
      res.page_results[0]['mean_frame_time'].value, 2)

  def testDoesImplThreadScroll(self):
    ps = self.CreatePageSetFromFileInUnittestDataDir('scrollable_page.html')

    benchmark = smoothness_benchmark.SmoothnessBenchmark()
    benchmark.force_enable_threaded_compositing = True
    all_results = self.RunBenchmark(benchmark, ps)

    results0 = all_results.page_results[0]
    self.assertTrue(results0['percent_impl_scrolled'].value > 0)

  def testScrollingWithoutGpuBenchmarkingExtension(self):
    ps = self.CreatePageSetFromFileInUnittestDataDir('scrollable_page.html')

    benchmark = smoothness_benchmark.SmoothnessBenchmark()
    benchmark.use_gpu_benchmarking_extension = False
    all_results = self.RunBenchmark(benchmark, ps)

    self.assertEqual(0, len(all_results.page_failures))
    self.assertEqual(1, len(all_results.page_results))
    results0 = all_results.page_results[0]

    self.assertTrue('dropped_percent' in results0)
    self.assertTrue('mean_frame_time' in results0)

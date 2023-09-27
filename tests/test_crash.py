from __future__ import print_function
import deepfuzzy_base
import logrun


class CrashTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/Crash"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("Passed: Crash_SegFault" in output)
    foundCrashSave = False
    for line in output.split("\n"):
      if ("Saved test case" in line) and (".crash" in line):
        foundCrashSave = True
    self.assertTrue(foundCrashSave)


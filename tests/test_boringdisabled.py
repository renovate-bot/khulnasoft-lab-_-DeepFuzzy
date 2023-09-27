from __future__ import print_function
import logrun
import deepfuzzy_base

class BoringDisabledTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/BoringDisabled"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("Passed: CharTest_BoringVerifyCheck" in output)
    self.assertTrue("Failed: CharTest_VerifyCheck" in output)

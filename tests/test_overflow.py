from __future__ import print_function
import logrun
import deepfuzzy_base


class OverflowTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "--timeout", "15", "build/examples/IntegerOverflow"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("Failed: SignedInteger_AdditionOverflow" in output)
    self.assertTrue("Passed: SignedInteger_AdditionOverflow" in output)
    self.assertTrue("Failed: SignedInteger_MultiplicationOverflow" in output)
    self.assertTrue("Passed: SignedInteger_MultiplicationOverflow" in output)

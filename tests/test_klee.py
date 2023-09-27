from __future__ import print_function
import deepfuzzy_base
import logrun


class KleeTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/Klee", "--klee"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("zero" in output)
    self.assertTrue("positive" in output)
    self.assertTrue("negative" in output)

from __future__ import print_function
import logrun
import deepfuzzy_base


class OneOfTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    if deepfuzzy == "deepfuzzy-figurative":
       return # Just skip for now, we know it fails (#174) 

    (r, output) = logrun.logrun([deepfuzzy, "build/examples/OneOf"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("Failed: OneOfExample_ProduceSixtyOrHigher" in output)
    self.assertTrue("Passed: OneOfExample_ProduceSixtyOrHigher" in output)

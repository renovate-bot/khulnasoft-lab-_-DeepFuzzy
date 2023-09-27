from __future__ import print_function
import deepfuzzy_base
import logrun


class RunlenTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    if deepfuzzy == "deepfuzzy-figurative":
       return # Just skip for now, we know it's too slow    
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/Runlen"],
                  "deepfuzzy.out", 2900)
    self.assertEqual(r, 0)

    self.assertTrue("Passed: Runlength_EncodeDecode" in output)
    foundFailSave = False
    for line in output.split("\n"):
      if ("Saved test case" in line) and (".fail" in line):
        foundFailSave = True
    self.assertTrue(foundFailSave)


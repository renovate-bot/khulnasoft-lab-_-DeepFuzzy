from __future__ import print_function
import logrun
import deepfuzzy_base


class TakeOverTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/TakeOver", "--take_over"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("hi" in output)
    self.assertTrue("bye" in output)
    self.assertTrue("was not greater than" in output)

    foundPassSave = False
    for line in output.split("\n"):
      if ("Saved test case" in line) and (".pass" in line):
        foundPassSave = True
    self.assertTrue(foundPassSave)

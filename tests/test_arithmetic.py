from __future__ import print_function
import logrun
import deepfuzzy_base


class ArithmeticTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/IntegerArithmetic", "--num_workers", "4"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("Failed: Arithmetic_InvertibleMultiplication_CanFail" in output)
    self.assertTrue("Passed: Arithmetic_AdditionIsCommutative" in output)
    self.assertFalse("Failed: Arithmetic_AdditionIsCommutative" in output)
    self.assertTrue("Passed: Arithmetic_AdditionIsAssociative" in output)
    self.assertFalse("Failed: Arithmetic_AdditionIsAssociative" in output)
    self.assertTrue("Passed: Arithmetic_InvertibleMultiplication_CanFail" in output)

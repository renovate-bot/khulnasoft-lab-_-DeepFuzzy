from __future__ import print_function
import logrun
import deepfuzzy_base


class PrimesTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "--timeout", "15", "build/examples/Primes"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("Failed: PrimePolynomial_OnlyGeneratesPrimes" in output)
    self.assertTrue("Failed: PrimePolynomial_OnlyGeneratesPrimes_NoStreaming" in output)

    # TODO(alan): determine why passed cases aren't being logged to stdout
    #self.assertTrue("Passed: PrimePolynomial_OnlyGeneratesPrimes" in output)
    #self.assertTrue("Passed: PrimePolynomial_OnlyGeneratesPrimes_NoStreaming" in output)

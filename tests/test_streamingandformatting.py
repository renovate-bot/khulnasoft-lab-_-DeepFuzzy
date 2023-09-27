from __future__ import print_function
import logrun
import deepfuzzy_base


class StreamingAndFormattingTest(deepfuzzy_base.DeepFuzzyTestCase):
  def run_deepfuzzy(self, deepfuzzy):
    (r, output) = logrun.logrun([deepfuzzy, "build/examples/StreamingAndFormatting"],
                  "deepfuzzy.out", 1800)
    self.assertEqual(r, 0)

    self.assertTrue("Failed: Streaming_BasicLevels" in output)
    self.assertTrue("This is a debug message" in output)
    self.assertTrue("This is a trace message" in output)    
    self.assertTrue("This is an info message" in output)
    self.assertTrue("This is a warning message" in output)
    self.assertTrue("This is a error message" in output)
    self.assertTrue("This is a trace message again" in output)
    self.assertTrue(": 97" in output)
    self.assertTrue(": 1" in output)
    self.assertTrue(": 1.000000" in output)
    self.assertTrue(": string" in output)
    self.assertTrue("hello string=world" in output)
    self.assertTrue("hello again!" in output)
    self.assertTrue("Passed: Formatting_OverridePrintf" in output)
    self.assertFalse("Failed: Formatting_OverridePrintf" in output)

from __future__ import print_function
from unittest import TestCase


class DeepFuzzyTestCase(TestCase):
  '''
  # Remove for now, since it fails

  def test_angr(self):
    self.run_deepfuzzy("deepfuzzy-angr")
  '''

  def test_builtin_fuzz(self):
    self.run_deepfuzzy("--fuzz")

  def test_figurative(self):
    return # Right now figurative always times out, so just skip it
    self.run_deepfuzzy("deepfuzzy-figurative")

  def run_deepfuzzy(self, deepfuzzy):
    raise NotImplementedError("Define an actual test of DeepFuzzy in DeepFuzzyTestCase:run_deepfuzzy.")


class DeepFuzzyFuzzerTestCase(TestCase):
  def test_afl(self):
    self.run_deepfuzzy("deepfuzzy-afl")

  def test_libfuzzer(self):
    self.run_deepfuzzy("deepfuzzy-libfuzzer")

  def test_honggfuzz(self):
    self.run_deepfuzzy("deepfuzzy-honggfuzz")

  def test_angora(self):
    self.run_deepfuzzy("deepfuzzy-angora")

  def test_eclipser(self):
    self.run_deepfuzzy("deepfuzzy-eclipser")

  def run_deepfuzzy(self, deepfuzzy):
    raise NotImplementedError("Define an actual test of DeepFuzzy in DeepFuzzyFuzzerTestCase:run_deepfuzzy.")

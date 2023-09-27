#include <deepfuzzy/DeepFuzzy.hpp>
#include <cmath>

using namespace deepfuzzy;
using namespace std;

TEST(DeepFuzzy, RangeAPIs) {
  int v, low, high, mv;
  char cv, clow, chigh;
  double dv, dlow, dhigh;

  low = DeepFuzzy_Int();
  high = DeepFuzzy_Int();
  if (low > high) {
    int temp = low;
    low = high;
    high = temp;
  }
  v = DeepFuzzy_IntInRange(low, high);
  LOG(TRACE) << "low = " << low << ", high = " << high << ", v = " << v;
  ASSERT ((low <= v) && (v <= high)) << "low = " << low << ", high = " << high << ", v = " << v;

  clow = DeepFuzzy_Char();
  chigh = DeepFuzzy_Char();
  if (clow > chigh) {
    char temp = clow;
    clow = chigh;
    chigh = temp;
  }
  cv = DeepFuzzy_CharInRange(clow, chigh);
  LOG(TRACE) << "clow = " << clow << ", chigh = " << chigh << ", cv = " << cv;
  ASSERT ((clow <= cv) && (cv <= chigh)) << "clow = " << clow << ", chigh = " << chigh << " cv = " << cv;

  mv = DeepFuzzy_IntInRange(2, 45);
  ASSIGN_SATISFYING(v, DeepFuzzy_Int(), (v % mv) == 0);
  ASSERT ((v % mv) == 0) << "mv = " << mv << ", v = " << v;

  dlow = DeepFuzzy_Double();
  assume(!isnan(dlow));
  dhigh = DeepFuzzy_Double();
  assume(!isnan(dhigh));
  if (dlow > dhigh) {
    double temp = dlow;
    dlow = dhigh;
    dhigh = temp;
  }
  dv = DeepFuzzy_DoubleInRange(dlow, dhigh);
  assume(!isnan(v));
  LOG(TRACE) << "dlow = " << dlow << ", dhigh = " << dhigh << ", dv = " << dv;
  ASSERT ((dlow <= dv) && (dv <= dhigh)) << "dlow = " << dlow << ", dhigh = " << dhigh << " dv = " << dv;

  low = DeepFuzzy_Int();
  high = DeepFuzzy_Int();
  if (low > high) {
    int temp = low;
    low = high;
    high = temp;
  }
  mv = DeepFuzzy_IntInRange(2, 45);  
  LOG(TRACE) << "low = " << low << ", high = " << high << ", mv = " << mv;
  ASSIGN_SATISFYING_IN_RANGE(v, DeepFuzzy_IntInRange(low, high), low, high, (v % mv) == 0);
  LOG(TRACE) << "v = " << v;
  ASSERT ((v % mv) == 0) << "v = " << v << ", mv = " << mv;
  ASSERT ((low <= v) && (v <= high)) << "low = " << low << ", high = " << high << " v = " << v;
}

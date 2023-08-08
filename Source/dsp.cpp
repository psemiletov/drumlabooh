#include "dsp.h"


double db_scale;

void init_db()
{
  db_scale = log (10.0) * 0.05;
}


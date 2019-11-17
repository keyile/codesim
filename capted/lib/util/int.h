#pragma once

namespace capted{

#ifdef CAPTED_LARGE_TREES
typedef int64_t Integer;
#else
typedef int32_t Integer;
#endif

}

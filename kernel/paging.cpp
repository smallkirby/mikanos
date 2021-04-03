#include"paging.hpp"
#include"asmfunc.h"

#include<array>

namespace{
  const uint64_t kPageSize4K = 0x1000;
  const uint64_t kPageSize2M = 0x200 * kPageSize4K;
  const uint64_t kPageSize1G = 0x200 * kPageSize2M;

  alignas(kPageSize4K) std::array<uint64_t, 0x200> pml4_table;    // Page-map level-4 table
  alignas(kPageSize4K) std::array<uint64_t, 0x200> pdp_table;     // Page-directory pointer table. One page-directory can map 2MiB*0x200==1GiB memory.
  alignas(kPageSize4K) std::array<std::array<uint64_t, 0x200>,  kPageDirectoryCount> page_directory;  // page directory
}

void SetupIdentityPageTable(void)
{
  // set only PML4's as pointer to PDP table. The rests are NULL.
  pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table[0]) | 0x003;
  // init each PDP table's entries.
  for(int i_pdpt = 0; i_pdpt != page_directory.size(); ++i_pdpt){
    // use 2MiB page
    pdp_table[i_pdpt] = reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
    for(int i_pd = 0; i_pd != 0x200; ++i_pd){
      page_directory[i_pdpt][i_pd] = i_pdpt * kPageSize1G + i_pd * kPageSize2M | 0x083;
    }
  }
  SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0]));
}
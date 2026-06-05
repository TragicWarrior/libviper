file(REMOVE_RECURSE
  ".1"
  "libvdk.pdb"
  "libvdk.so"
  "libvdk.so.1"
  "libvdk.so.1.0.0"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/vdk-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()

file(REMOVE_RECURSE
  "libvdk.a"
  "libvdk.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/vdk-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()

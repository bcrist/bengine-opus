module { name = 'opus',
   projects = {
      lib {
         src = {
            'src/*.cpp',
            pch = 'src/pch.cpp'
         },
         preprocessor = {
            'BE_OPUS_IMPL'
         }
      }
   }
}

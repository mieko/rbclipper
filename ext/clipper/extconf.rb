require 'mkmf'

CONFIG['LDSHARED'] = "$(CXX) -shared"

dir_config('clipper')
create_makefile('clipper')

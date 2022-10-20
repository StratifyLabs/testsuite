/*
Copyright 2016-2018 Tyler Gilbert

         Licensed under the Apache License, Version 2.0 (the "License");
         you may not use this file except in compliance with the License.
         You may obtain a copy of the License at

                         http://www.apache.org/licenses/LICENSE-2.0

         Unless required by applicable law or agreed to in writing, software
         distributed under the License is distributed on an "AS IS" BASIS,
         WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and
         limitations under the License.
*/

#include <sys/Cli.hpp>
#include <fs/File.hpp>


#include "sl_config.h"

using namespace sys;
using namespace fs;

int main(int argc, char *argv[]) {
  Cli cli(argc, argv);

  if( const auto option = cli.get_option("orphan"); option.is_empty() == false ){
    File(File::IsOverwrite::yes, "/home/orphan.txt").write(option);
  }

  if( const auto option = cli.get_option("wait"); option.is_empty() == false ){
    const auto duration = option.to_integer();
    wait(duration * 1_milliseconds);
    return duration;
  }

  if( cli.get_option("user") == "true" ){
    const auto ppid = getppid();
    kill(ppid, SIGUSR1);
    return 190;
  }

  if( cli.get_option("infinite") == "true" ){
    while(1){
      wait(1_seconds);
    }
    return 7;
  }

  return 0;
}



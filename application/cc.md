Update Sumodules
================
?? Not sure if it is --remote or --recursive or both.
git submodule update --recursive
use the command above to update all submodules the project uses.

Detached heads and submodules
=============================
Reference:
https://stackoverflow.com/questions/18770545/why-is-my-git-submodule-head-detached-from-master/55570998#55570998

Navigate to the submodule folder:
  cd external\library
  git checkout main
  git pull
Afterwards commit and push in visual studio code and then you can push into the 
module like normal. Understandibly this last step would be different if you are
doing it from the command line, the reference thread has the answer for that.
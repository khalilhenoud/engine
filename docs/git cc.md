Remove a Submodule
==================
    git submodule deinit -f external/library
    git rm external/library
then commit.
@see https://stackoverflow.com/questions/1260748/how-do-i-remove-a-submodule

Add a Submodule
===============
    git submodule add <repository-url> <submodule-path>
If the git directory was previously used but discarded and you want to
re-activate it, use:
    git submodule add --force <repository-url> <submodule-path>

To populate a fresh repo
========================
    git init
    git submodule add https://github.com/khalilhenoud/library.git external/library
    git submodule add https://github.com/khalilhenoud/math.git external/math
    git submodule add https://github.com/khalilhenoud/collision.git external/collision
    git submodule add https://github.com/khalilhenoud/windowing.git external/windowing
    git submodule add https://github.com/khalilhenoud/entity.git external/entity
    git submodule add https://github.com/khalilhenoud/loaders.git external/loaders
    git submodule add https://github.com/khalilhenoud/renderer.git external/renderer
    git add .
    git commit -m "first commit"
    git branch -M main
    git remote add origin https://github.com/khalilhenoud/game.git
    git push -u origin main

Update Submodules
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

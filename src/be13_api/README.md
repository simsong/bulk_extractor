be13_api
========

API for bulk_extractor version 1.3


Set remote origin:
  http://stackoverflow.com/questions/7259535/git-setting-up-a-remote-origin
  git remote add origin git@github.com:simsong/be13_api.git

http://stackoverflow.com/questions/5828324/update-git-submodule

Update to this repository to master:
    git pull origin master

Push changes in this repository to master:

    git push origin master

If you get this error:

error: failed to push some refs to 'git@github.com:simsong/be13_api.git'
hint: Updates were rejected because a pushed branch tip is behind its remote
hint: counterpart. If you did not intend to push that branch, you may want to
hint: specify branches to push or set the 'push.default' configuration
hint: variable to 'current' or 'upstream' to push only the current branch.
$ 

Do this:
$ git checkout -b tmp  ; git checkout master ; git merge tmp ; git branch -d tmp ; git push git@github.com:simsong/be13_api.git master

Extended:

$ git checkout -b tmp 
Switched to a new branch 'tmp'
$ git checkout master
Switched to branch 'master'
Your branch is behind 'origin/master' by 8 commits, and can be fast-forwarded.
$ git merge tmp
Updating 0dbc904..74aca46
Fast-forward
 CODING_STANDARDS.txt |    4 ++++
 README.md            |   12 +++++++++++-
 bulk_extractor_i.h   |   14 ++++++++++++--
 pcap_fake.cpp        |   13 ++++++++++++-
 pcap_fake.h          |    5 +++++
 sbuf.cpp             |   36 +++++++++++++++++++++---------------
 sbuf.h               |   41 ++++++++++++++++++++++++-----------------
 7 files changed, 89 insertions(+), 36 deletions(-)
$ git branch -d tmp
Deleted branch tmp (was 74aca46).
$ git push git@github.com:simsong/be13_api.git master
Counting objects: 7, done.
Delta compression using up to 4 threads.
Compressing objects: 100% (4/4), done.
Writing objects: 100% (4/4), 562 bytes, done.
Total 4 (delta 2), reused 0 (delta 0)
To git@github.com:simsong/be13_api.git
   2d14a08..74aca46  master -> master
$ 


Summary:
$ git checkout -b tmp; git checkout master; git merge tmp; git branch -d tmp

Followed by:

$ git push git@github.com:simsong/be13_api.git master

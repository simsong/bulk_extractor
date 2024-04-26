docker run -dit --name fedora-container fedora
# docker exec -it fedora-container yum install -y git
docker cp paths.bash fedora-container:paths.bash
docker cp CONFIGURE_FEDORA39.bash fedora-container:CONFIGURE_FEDORA39.bash
docker cp CONFIGURE_FEDORA39_win64.bash fedora-container:CONFIGURE_FEDORA39_win64.bash
docker exec -it fedora-container /bin/bash CONFIGURE_FEDORA39.bash

echo note - Updater the script not to reinstall libewf if it is installed
echo 
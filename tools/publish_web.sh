#!/bin/zsh
# Publishes PokeWebGochi (web/play/, built by webgame/build.sh) to its own GitHub Pages site,
# https://pokewebgochi.3dprintsos.com (repo FedorSosnin/pokewebgochi). The game lives at that one
# address only: a browser keeps saves per site, so a second address would mean a second pet.
#   tools/publish_web.sh
set -e
cd "${0:A:h}/.."
REPO=FedorSosnin/pokewebgochi
DOMAIN=pokewebgochi.3dprintsos.com
SITE=build/pokewebgochi-site
GIT=(git -c credential.helper= -c 'credential.helper=!gh auth git-credential')

[[ -f web/play/index.html ]] || { echo "build it first: webgame/build.sh"; exit 1; }
if [[ ! -d $SITE/.git ]]; then
  rm -rf $SITE
  $GIT clone -q https://github.com/$REPO.git $SITE
fi
$GIT -C $SITE pull -q --ff-only 2>/dev/null || true
rsync -a --delete --exclude .git --exclude CNAME --exclude README.md web/play/ $SITE/
echo $DOMAIN > $SITE/CNAME
cat > $SITE/README.md <<EOF
# PokeWebGochi

The browser version of [PokeKnomiGochi](https://github.com/FedorSosnin/PokeKnomiGochi), published at
https://$DOMAIN. These are built files: the source and how to build them are in that repository
(\`webgame/\`), and this repository is updated by its \`tools/publish_web.sh\`.

Based on [TamaPoke](https://github.com/socquique/TamaPoke) by Quique Tortosa. Sprites from PMDCollab,
CC BY-NC 4.0. Pokémon © Nintendo / Game Freak / The Pokémon Company. Personal, non-commercial use only.
EOF
touch $SITE/.nojekyll
$GIT -C $SITE add -A
if $GIT -C $SITE diff --cached --quiet; then echo "nothing new to publish"; exit 0; fi
VER=$(sed -n 's/^#define FW_VERSION "\(.*\)"/\1/p' PokeKnomiGochi.ino)
$GIT -C $SITE commit -q -m "PokeWebGochi $VER ($(cat web/play/version.json | tr -d '\n'))"
$GIT -C $SITE push -q origin HEAD:main
echo "published $VER to https://$DOMAIN"

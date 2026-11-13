#!/bin/zsh
# Publishes the hand-over page (webgame/handover/) to the web game's old address,
# https://pokewebgochi.3dprintsos.com (repo FedorSosnin/PokeWebGochi): it reads the pet that address
# kept in the browser and takes it to https://catch151.3dprintsos.com (Fedor: "current players on web
# should all go to the new catch151 links"). The old address stays up for that, and as the clock of the
# Mac and Windows apps up to 2.1.4 (host/host.h's HOST_TIME_URL then).
#   tools/publish_web_handover.sh
set -e
cd "${0:A:h}/.."
REPO=FedorSosnin/PokeWebGochi
DOMAIN=pokewebgochi.3dprintsos.com
SITE=build/pokewebgochi-handover-site
GIT=(git -c credential.helper= -c 'credential.helper=!gh auth git-credential')

if [[ ! -d $SITE/.git ]]; then
  rm -rf $SITE
  $GIT clone -q https://github.com/$REPO.git $SITE
fi
$GIT -C $SITE pull -q --ff-only 2>/dev/null || true
[[ -n $(git -C $SITE config user.email) ]] || git -C $SITE config user.email "$(git config user.email)"
[[ -n $(git -C $SITE config user.name) ]] || git -C $SITE config user.name "$(git config user.name)"
rsync -a --delete --exclude .git --exclude CNAME webgame/handover/ $SITE/
echo $DOMAIN > $SITE/CNAME
touch $SITE/.nojekyll
$GIT -C $SITE add -A
if $GIT -C $SITE diff --cached --quiet; then echo "nothing new to publish"; exit 0; fi
$GIT -C $SITE commit -q -m "Catch 151: this address hands each pet over to https://catch151.3dprintsos.com"
$GIT -C $SITE push -q origin HEAD:main
echo "published the hand-over to https://$DOMAIN"

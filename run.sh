PLAYER=foo
GAME=bar

cleos push action gameoflife create "{\"user\":\"$PLAYER\", \"game\":\"$GAME\", \"num_rows\":20, \"num_cols\":20, \"seed\":19}" --permission $PLAYER@active

while true; do
    cleos push action gameoflife step "{\"user\":\"$PLAYER\", \"game\":\"$GAME\"}" --permission $PLAYER@active
    cleos get table gameoflife $PLAYER boards
    sleep .5
done

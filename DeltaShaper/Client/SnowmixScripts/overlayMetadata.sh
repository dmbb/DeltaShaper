echo 'command create Show
      image overlay 1
      loop
    command end ' | nc localhost 9999

echo 'feed shift 2 639 479 ' | nc localhost 9999
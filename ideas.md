### Ideas for the mothaheckin' project
The gyroscope only needs to know things in 2D. This is because we are going to implement the gyroscope to make morse letters. Morse letters
consist of - and . . To make a message we need to define what is a long and what is a short. We have 360 degrees to our use case and probably should 
define the constraints within this limitation. There should be threshold values for our device to recognize the difference between a long and a short letter.
We should also define the way we are going to confirm if a letter a long or short is accepted , eg. pressing a button.

Z is rotation on table

Oikea viiva, vasen piste

# International morse coden mukaan:
1. len of dot is 1 unit
2. len of dash is 3 units
3. space between parts of same letter is 1 unit
4. space between letters is 3 units
5. space between words is 7 units

Maksimi koko sille sana bufferille 6 koska pisin kirjain on 5 pistettä ja viivaa

Laitetaan kaikki kirjaimet ja numerot johonkin dictionaryn tapasee? Ihana tree structure tulee noista jos haluaa hifistellä

Esim:
A -> * -
B -> - * * *
jne jne.

Tavallaan vois vasemman flikin ja oikean flikin laittaa eri velocityn koska ei oikealla kadella pysty oikealla flikkaan niin hyvin kuin vasemmalle

## Screen Printing
1. Append - | . to symbol buffer + terminator and redraw with them
2. if len > 0 use lookup table and append the char to the word buffer
3. Reset symbol buf to 0 + terminate and redraw
4. loop
5. profit
### Ideas for the mothaheckin' project
The gyroscope only needs to know things in 2D. This is because we are going to implement the gyroscope to make morse letters. Morse letters
consist of - and . . To make a message we need to define what is a long and what is a short. We have 360 degrees to our use case and probably should 
define the constraints within this limitation. There should be threshold values for our device to recognize the difference between a long and a short letter.
We should also define the way we are going to confirm if a letter a long or short is accepted , eg. pressing a button.

## Origo

## Timestamp Nuutti

Z is rotation on table

TODO: Gyro calibration in sdk.c
TODO: JASPER katso screen ja sisäistä

Oikea viiva, vasen piste

International morse coden mukaan:
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



Eli ajattelin näin:
1. Ihminen kääntää joko oikealle tai vasemalle
2. Saadaan viiva tai piste queueueueueueuhen
3. Ihminen palauttaa levyn välille 345 < x < 15 astetta
4. Ihminen saa antaa uuden pisteen tai kirjaimen
5. repeat until väli
    5.1. Välilyönti joko levyn nosto pöydältä tai nappi ihsm
6. Profit


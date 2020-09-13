/* 
 * This is simple code for door lock control. 
 * 
 * For usage you need to setup mifare card with keys A and B.
 *  Set A for reading and B for writing.
 *  Keys have to be set in array keyA and keyB
 *  
 *  Principle is simple. At first start array of unique random numbers is generated, then it is saved.
 *  With each door opening random value at random byte in first block is changend in patternt that was generated.
 *  This gave us aprox 4000 possibilities. If we limit time beetween reads on 20 seconds, attacker would neet ~22 hours to determine whole pattern which we use.
 *  Value and number of byte is saved for each card. If card is compromised then copied card stops working, if genuine card is read before copied one and vice versa.
 *  In either case we can easily determine which card was compromised, because it stop working for valid user or attacker for 4000 iterations.
 *  
 *  If connecting to WiFi is unsuccessfull, ESP will switch to AP mode and continue.
 *  
 *  shuffle.c is C app that you can use for generation of array of unique random numbers from 1 to MAX.
 *  
 *  generate_random_string_bash.c is bash comand used for generating pwd
 *  
 *  
 *   
 */

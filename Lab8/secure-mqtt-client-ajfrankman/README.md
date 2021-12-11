# Secure MQTT Client
# STILL WORKING ON THE WRITTEN PART!!!

https://ecenetworking.byu.edu/426/labs/secure-mqtt-client/

## Questions

#### 1. Why would we want end-to-end security with MQTT? What attacks does end-to-end security protect against with MQTT? Be specific about potential attacks a compromised broker could perform.
If we don't have end to end encryption, then the broker could be intercepting our messages. That is fine as long as your trust or maintain the broker but you can't always do that. Not to mention, if we want to send a message to only one person but someone else subscribes to our topic, even if they intercept, they won't be able to make sense of the message.

#### 2. Where are `p` and `g` (from the [Diffie-Hellman key exchange](https://en.wikipedia.org/wiki/Diffie–Hellman_key_exchange)) used in the code? Is `p` and `g` sent in the key exchange? If not, how are they agreed upon?
p - is a very large prime number.
g - is a smaller than p number.
In our code 'p' and 'g' are part of the .pem file used to generate a private and public key with the generate_keyset() function. But because of how Diffie Hellman works, you can just contact someone to agree on some numbers 'p' and 'g' then use those numbers in a the algorithm.

#### 3. Who initiates the key exchange? Describe how the keys are exchanged in this lab.

In our code, we initiate it with a another client who is expecting to partake in the exchange. In the real world, you can just tell someone you want to start a diffie Hellman exchange and then you can go ahead and do it. Agreeing to start one in the 'public' isn't actually bad. 

#### 4. Why do we hash the secret key?

[put answer here]

#### 5. Is this MQTT key exchange protocol susceptible to any vulnerabilities? If so, explain.

If there are multiple clients subscribed to a topic, we could be getting back different keys and if we were set up to differentiate between clients that would be fine but we aren't. On that note, if someone knew knew your 'p' and 'g' someone could pretend to be someone they are not, you could exchange keys, and then whatever you send them might be something you meant to send to someone else. 


-- Pub-Sub addresses an old messaging problem:
-- multicast or group messaging.
-- PUB sends each message to "all of many",
-- whereas PUSH and DEALER rotate messages to
-- "one of many".
-- Pub-Sub is aimed at scalability, large volumes
-- of data, sent rapidly to many recipients.
-- To get scalability, pub sub gets rid of
-- back-chatter. Recipients don't talk back
-- to senders. We remove any possibility to
-- coordinate senders and receivers. That is,
-- publishers can't tell when subscribers are
-- successfully connected. Subscribers can't
-- tell publishers anything that would allow
-- publishers to control the rate of messages
-- they send.
-- When we need back-chatter, we can either switch
-- to using ROUTER-DEALER or we can add a separate
-- channel for synchronization.
-- Pub-sub is like a radio broadcast, you miss
-- everything before you join. The classic
-- failure cases are: subscribers join late,
-- subscribers can fetch messages too slowly,
-- subscribers can drop off and lose messages,
-- networks can become overloaded, or too slow.
--

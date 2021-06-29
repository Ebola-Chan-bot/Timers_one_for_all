Make full use of all your hardware timers on your Arduino board. 

The only library you can choose any hardware timer you like to use in your timing function. Tones, square waves, delayed tasks, timed repetitive tasks are provided as building blocks for your own sophisticated multi-timer tasks. My library hides hardware register details for you.

Currently all APIs are function templates, which means all arguments must be known as constants at compile-time. Non-template versions will be gradually added if I have more free time. However, you're welcomed to modify my code - not so difficult to understand if you are familiar with ATMega registers.
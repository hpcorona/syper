syper
=====

## A little bit of history...

First of all... how does syper was born?

It all started when i had to implement [ZeroMQ](http://zeromq.org/) at work.

When i finished implementing it, our program ran flawlessly, cummunication
between services and nodes were very easy to implement. It was implemented
in a Client/Server environment (a really big Desktop Application).

But i also do Web Development. And somehow i asked myself... Is there any
web server that delegates it's requests to 0mq nodes? and Google told me:
Yes, there is one... It's name was Mongrel2.

[Mongrel2](http://mongrel2.org/) is a web server that focus on delegating
the requests to 0mq sockets. Plain and simple. I said "i'll use that", but
oh wait, big problem... I need it for Windows, because, you know, the market
in which i move is on Windows, so, yeah, that was pretty sad that Mongrel2
wasn't available on windows.

So i told myself "how difficult could it be to build one web server like
Mongrel2?"... I was crazy, and give it a tought for several weeks.

I researched on free time. And research and research. Then somehow i
thinked about [FastCGI](http://www.fastcgi.com/drupal/), and started
reading it's documentation.

And then an idea came up. Why should i focus on what lot's of web services
already do? So i build Syper.

## Yeah, but what is Syper?

Syper is a very simple dynamic linked library that uses ZeroMQ and FastCGI.

It's simple, it waits for Requests from Apache, nginx, IIS 7, or any other
FastCGI enabled web server, and then it passes that request to a ZeroMQ node.
When the ZeroMQ node responds, it passes the response back to the FastCGI
enabled web server.

Plan and simple.

## Is this a Mongrel2 for any web server?

No, and it will never be. I belive Mongrel2 has better architecture... Talking
about technicisms, Mongrel2 uses PUSH and SUB 0mq sockets, and Syper uses
the ultimate DEALER/ROUTER combo, which has it's downsides but it's very
easy to maintain, anyone can understand it.

Also, Mongrel2 supports some kind of longpolling. Syper doesn't. Why? because
Syper don't have complete control over the web server, and Mongrel2 does. Yeah
it could be implemented on Syper, but it could be very "hacky" and it may make
things more difficult (for example i would need to change from DEALER/ROUTER to
PUSH/SUB as Mongrel2 does, and that's not all, lots of challenges arises).

## Why could i use Syper for?

You can use it to develop Web Applications using virtually any programming language
that supports calling plain C functions on a DLL or SO file, or any
language that can talk 0mq.

Maybe your language of choice already available for Web Development, and you
don't need Syper.

I build Syper to use [WinDev](http://www.windev.com/) to develop Web Applications.

## But hey, there's WebDev from PC Soft, just use that!!

Ok, i've been using WebDev since version 11. And I DON'T LIKE IT. And also,
it's expensive to deploy an application because of the licensing fee.

I develop the projects using it, but, i just don't like it.

But what i do like, is the easy data access modules that WinDev provides
for Desktop applications. It's not the fastest language of the world,
but the tools it provides for data access are great.

And data access is a big part of web development. So, how can i
do web development using only WinDev? the answer now is Syper.

## Platforms

The current version is only for Windows (XP SP 3 and up).

The Syper C++ DLL can be compiled on Linux and OSX with very little changes.
Actually i started it on OSX and tested it a lot using Xcode profiling tools,
and Apache Benchmarks to make sure it isn't leaking memory and CPU usage was
acceptable. Then i moved it to Visual Studio (i hate Visual Studio, that's
why i started on Xcode) and made a few changes to make it work on Windows.

Right now WinDev runs on Linux and Windows. So, i'll provide Makefiles
as soon as the Linux version is working in the near future. I want
to host a WinDev application using Syper, maybe just a pet project just to
make sure Syper is working well.

## Documentation

Sorry, the only documentation hint i can give you, is this:

Look at the syper.h file.

It should be very straightforward.

## License

(The MIT License)

Copyright (c) 2014 Hilario Pérez Corona &lt;hpcorona@gmail.com&gt;

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# mruby-msgpack   [![Build Status](https://travis-ci.org/suzukaze/mruby-msgpack.png?branch=master)](https://travis-ci.org/suzukaze/mruby-msgpack)
## Welcome to MessagePack for mruby

MessagePack for mruby is MessagePack implimented in mruby language.
```ruby
msg = [1, 2, 3].to_msgpack  #=> "\x93\x01\x02\x03"
MessagePack.unpack(msg)     #=> [1, 2, 3]
```

This is early vesion. Please check the methods that work in test folder.

## Platform

I test MessagePack for mruby in mac OSX 10.8.4. In the future it will work in Windows and Linux OS.

## Getting Started

1. Download MessagePack for mruby at the command prompt:

        git clone https://github.com/suzukaze/mruby-msgpack.git

2. Add config.gem line to `build_config.rb`
```ruby
MRuby::Build.new do |conf|

  # ...(snip)...
  conf.gem :git => 'https://github.com/suzukaze/mruby-msgpack.git'
end
```

3. Test at the command prompt:

        rake test

4. Build at the command prompt:

        rake

## msgpack-ruby commit
mruby-msgpack is based on [msgpack-ruby](https://github.com/msgpack/msgpack-ruby)(`a7c2dc34ef69c9132167e38009baa8420c460c9b`)

## Contributing

I encourage you to contribute to MessagePack for mruby!

## License

Author : Jun Hiroe

Copyrigh : Copyright (c) 2013 Jun Hiroe

License : MIT License

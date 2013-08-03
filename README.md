## Welcome to MessagePack for mruby

MessagePack for mruby is MessagePack implimented in mruby language.
```ruby
msg = [1, 2, 3].to_msgpack  #=> "\x93\x01\x02\x03"
MessagePack.unpack(msg)     #=> [1, 2, 3]
```

This is early vesion. Please check the methods that work in test foloder.

## Plathome
I test MessagePack for mruby in mac OSX 10.8.4. In the future it will work in Windows and Linux OS.

## Getting Started

1. Download MessagePack for mruby at the command prompt:

        git clone https://github.com/suzukaze/mruby-msgpack.git

2. Add config.gem line to `build_config.rb`
```ruby
MRuby::Build.new do |conf|

  # ...(snip)...
  config.gem :git => 'https://github.com/suzukaze/mruby-msgpack.git'
end
```

3. Test:

        rake test

4. Build:

    	rake



## Contributing

I encourage you to contribute to MessagePack for mruby!

## License

Author : Jun Hiroe

Copyrigh : Copyright (c) 2013 Jun Hiroe

License : MIT License

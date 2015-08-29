##
# Unpacker Test

# Start -- from MessagePack-ruby --
assert('Unacker', 'read_array_header succeeds') do
  unpacker = MessagePack::Unpacker.new
  unpacker.feed("\x91")
  assert_equal(unpacker.read_array_header, 1)
end

=begin
assert('Unacker', 'read_array_header fails') do
  unpacker = MessagePack::Unpacker.new
  unpacker.feed("\x81")
  #assert_raise
end
=end

assert('Unacker', 'read_map_header succeeds') do
  unpacker = MessagePack::Unpacker.new
  unpacker.feed("\x81")
  assert_equal(unpacker.read_map_header, 1)
end

=begin
assert('Unacker', 'read_map_header fails') do
  unpacker = MessagePack::Unpacker.new
  unpacker.feed("\x91")
  #assert_raise
end
=end

assert('Unacker', 'skip_nil succeeds') do
  unpacker = MessagePack::Unpacker.new
  unpacker.feed("\xc0")
  assert_equal(unpacker.skip_nil, true)
end

assert('Unacker', 'skip_nil fails') do
  unpacker = MessagePack::Unpacker.new
  unpacker.feed("\x90")
  assert_equal(unpacker.skip_nil, false)
end

=begin
assert('Unacker', 'skip skips objects') do
  packer = MessagePack::Packer.new
  packer.write(1)
  packer.write(2)
  packer.write(3)
  packer.write(4)
  packer.write(5)

  unpacker = MessagePack::Unpacker.new(packer.buffer)

  assert_equal(unpacker.read, 1)
#  unpacker.skip
#  assert_equal(unpacker.read, 3)
#  unpacker.skip
#  assert_equal(unpacker.read, 5)
end
=end

# End -- from MessagePack-ruby --

# Start -- MessagePack-mruby --

assert('MessagePack.unpack', 'Fixnum 1') do
  msg = MessagePack.unpack("\x01")
  assert_equal(1, msg)
end

assert('MessagePack.unpack', 'Fixnum 127') do
  msg = MessagePack.unpack("\x7f")
  assert_equal(127, msg)
end

assert('MessagePack.unpack', 'Fixnum -1') do
  msg = MessagePack.unpack("\xff")
  assert_equal(-1, msg)
end

assert('MessagePack.unpack', 'Fixnum -32') do
  msg = MessagePack.unpack("\xe0")
  assert_equal(-32, msg)
end

assert('MessagePack.unpack', 'Catergory-1 nil') do
  msg = MessagePack.unpack("\xc0")
  assert_nil(msg)
end

assert('MessagePack.unpack', 'Catergory-1 false') do
  msg = MessagePack.unpack("\xc2")
  assert_false(msg)
end

assert('MessagePack.unpack', 'Catergory-1 true') do
  msg = MessagePack.unpack("\xc3")
  assert_true(msg)
end

=begin
# Infinite loop!
assert('MessagePack.pack', 'Catergory-2 float') do
  msg = MessagePack.unpack("\xca\x3f\xf0\x00")
  assert_equal(1.0, msg)
end
=end

assert('MessagePack.pack', 'Catergory-2 double') do
  msg = MessagePack.unpack("\xcb\x3f\xf0" + "\x00" * 6)
  assert_equal(1.0, msg)
end

assert('MessagePack.unpack', 'Catergory-2 uint16 128') do
  msg = MessagePack.unpack("\xcc\x80")
  assert_equal(128, msg)
end

assert('MessagePack.unpack', 'Catergory-2 uint16 255') do
  msg = MessagePack.unpack("\xcc\xff")
  assert_equal(255, msg)
end

assert('MessagePack.unpack', 'Catergory-2 uint16 256') do
  msg = MessagePack.unpack("\xcd\x01\x00")
  assert_equal(256, msg)
end

assert('MessagePack.unpack', 'Catergory-2 uint16 65535') do
  msg = MessagePack.unpack("\xcd\xff\xff")
  assert_equal(65535, msg)
end

assert('MessagePack.unpack', 'Catergory-2 uint32 65536') do
  msg = MessagePack.unpack("\xce\x00\x01\x00\x00")
  assert_equal(65536, msg)
end

assert('MessagePack.unpack', 'Catergory-2 int8 -33') do
  msg = MessagePack.unpack("\xd0\xdf")
  assert_equal(-33, msg)
end

assert('MessagePack.unpack', 'Catergory-2 int8 -255') do
  msg = MessagePack.unpack("\xd1\xff\x01")
  assert_equal(-255, msg)
end

assert('MessagePack.unpack', 'Catergory-2 int16 -256') do
  msg = MessagePack.unpack("\xd1\xff\x00")
  assert_equal(-256, msg)
end

assert('MessagePack.unpack', 'Catergory-2 int32 -65535') do
  msg = MessagePack.unpack("\xd2\xff\xff\x00\x01")
  assert_equal(-65535, msg)
end

assert('MessagePack.unpack', 'Catergory-2 int32 -65536') do
  msg = MessagePack.unpack("\xd2\xff\xff\x00\x00")
  assert_equal(-65536, msg)
end

assert('MessagePack.unpack', 'Catergory-3 raw16 "a" * 32') do
  msg = MessagePack.unpack("\xda\x00\x20" + "\x61" * 32)
  assert_equal("a" * 32, msg)
end

assert('MessagePack.unpack', 'FixRaw ""') do
  msg = MessagePack.unpack("\xa0")
    assert_equal("", msg)
end

assert('MessagePack.unpack', 'FixRaw "a"') do
  msg = MessagePack.unpack("\xa1\x61")
    assert_equal("a", msg)
end

assert('MessagePack.unpack', 'FixRaw "1"') do
  msg = MessagePack.unpack("\xa1\x31")
    assert_equal("1", msg)
end

assert('MessagePack.unpack', 'FixRaw "a" * 31') do
  msg = MessagePack.unpack("\xbf" + "\x61" * 31)
  assert_equal("a" * 31, msg)
end

assert('MessagePack.unpack', 'FixArray []') do
  msg = MessagePack.unpack("\x90")
  assert_equal([], msg)
end

assert('MessagePack.unpack', 'FixArray [1, 2, 3]') do
  msg = MessagePack.unpack("\x93\x01\x02\x03")
  assert_equal([1, 2, 3], msg)
end

# Symbol is replaced string with.
assert('MessagePack.unpack', 'FixArray ["a", :b, 3]') do
  msg = MessagePack.unpack("\x93\xa1\x61\xa1\x62\x03")
  assert_equal(["a", "b", 3], msg)
end

# End -- MessagePack-mruby --


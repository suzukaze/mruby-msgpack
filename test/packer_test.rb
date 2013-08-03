##
# Packer Test

# Start -- from MessagePack-ruby --
assert('Packer', 'write') do
  packer = MessagePack::Packer.new
  packer.write([])
  assert_equal([0x90], packer.to_str.bytes.to_a)
end

assert('Packer', 'write_nil') do
  packer = MessagePack::Packer.new
  packer.write_nil
  assert_equal([0xc0], packer.to_str.bytes.to_a)
end

assert('Packer', 'write_array_header 0') do
  packer = MessagePack::Packer.new
  packer.write_array_header(0)
  assert_equal([0x90], packer.to_str.bytes.to_a)
end

assert('Packer', 'write_array_header 1') do
  packer = MessagePack::Packer.new
  packer.write_array_header(1)
  assert_equal([0x91], packer.to_str.bytes.to_a)
end

assert('Packer', 'write_map_header 0') do
  packer = MessagePack::Packer.new
  packer.write_map_header(0)
  assert_equal([0x80], packer.to_str.bytes.to_a)
end

assert('Packer', 'write_map_header 1') do
  packer = MessagePack::Packer.new
  packer.write_map_header(1)
  assert_equal([0x81], packer.to_str.bytes.to_a)
end
# End -- from MessagePack-ruby --

# Start -- MessagePack-mruby --

assert('MessagePack.pack', 'Fixnum 1') do
  msg = MessagePack.pack(1)
  assert_equal([1], msg.to_str.bytes.to_a)
end

assert('MessagePack.pack', 'Fixnum 127') do
  msg = MessagePack.pack(127)
  array = msg.to_str.bytes.to_a
  assert_equal([0x7f], array)
end

assert('MessagePack.pack', 'Fixnum -1') do
  msg = MessagePack.pack(-1)
  array = msg.to_str.bytes.to_a
  assert_equal([0xff], array)
end

assert('MessagePack.pack', 'Fixnum -32') do
  msg = MessagePack.pack(-32)
  array = msg.to_str.bytes.to_a
  assert_equal([0xe0], array)
end

assert('MessagePack.pack', 'Catergory-1 nil') do
  msg = MessagePack.pack(nil)
  array = msg.to_str.bytes.to_a
  assert_equal([0xc0], array)
end

assert('MessagePack.pack', 'Catergory-1 false') do
  msg = MessagePack.pack(false)
  array = msg.to_str.bytes.to_a
  assert_equal([0xc2], array)
end

assert('MessagePack.pack', 'Catergory-1 true') do
  msg = MessagePack.pack(true)
  array = msg.to_str.bytes.to_a
  assert_equal([0xc3], array)
end

assert('MessagePack.pack', 'Catergory-2 double') do
  msg = MessagePack.pack(1.0)
  array = msg.to_str.bytes.to_a
  assert_equal([203, 63, 240, 0, 0, 0, 0, 0, 0], array)
end

assert('MessagePack.pack', 'Catergory-2 uint16 128') do
  msg = MessagePack.pack(128)
  array = msg.to_str.bytes.to_a
  assert_equal([204, 128], array)
end

assert('MessagePack.pack', 'Catergory-2 uint16 255') do
  msg = MessagePack.pack(255)
  array = msg.to_str.bytes.to_a
  assert_equal([204, 255], array)
end

assert('MessagePack.pack', 'Catergory-2 uint16 256') do
  msg = MessagePack.pack(256)
  array = msg.to_str.bytes.to_a
  assert_equal([205, 1, 0], array)
end

assert('MessagePack.pack', 'Catergory-2 uint16 65535') do
  msg = MessagePack.pack(65535)
  array = msg.to_str.bytes.to_a
  assert_equal([205, 255, 255], array)
end

assert('MessagePack.pack', 'Catergory-2 uint32 65536') do
  msg = MessagePack.pack(65536)
  array = msg.to_str.bytes.to_a
  assert_equal([206, 0, 1, 0, 0], array)
end

assert('MessagePack.pack', 'Catergory-2 int8 -33') do
  msg = MessagePack.pack(-33)
  array = msg.to_str.bytes.to_a
  assert_equal([208, 223], array)
end

assert('MessagePack.pack', 'Catergory-2 int16 -255') do
  msg = MessagePack.pack(-255)
  array = msg.to_str.bytes.to_a
  assert_equal([209, 255, 1], array)
end

assert('MessagePack.pack', 'Catergory-2 int16 -256') do
  msg = MessagePack.pack(-256)
  array = msg.to_str.bytes.to_a
  assert_equal([209, 255, 0], array)
end

assert('MessagePack.pack', 'Catergory-2 int16 -65535') do
  msg = MessagePack.pack(-65535)
  array = msg.to_str.bytes.to_a
  assert_equal([210, 255, 255, 0, 1], array)
end

assert('MessagePack.pack', 'Catergory-2 int16 -65536') do
  msg = MessagePack.pack(-65536)
  array = msg.to_str.bytes.to_a
  assert_equal([210, 255, 255, 0, 0], array)
end

assert('MessagePack.pack', 'Catergory-3 raw16 "a" * 32') do
  msg = MessagePack.pack("a" * 32)
  array = msg.to_str.bytes.to_a
  assert_equal([218, 0, 32, 97, 97, 97, 97, 97, 97, 97, 97, 97,
                97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
                97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97], array)
end

assert('MessagePack.pack', 'FixRaw ""') do
  msg = MessagePack.pack("")
  array = msg.to_str.bytes.to_a
  assert_equal([160], array)
end

assert('MessagePack.pack', 'FixRaw "a"') do
  msg = MessagePack.pack("a")
  array = msg.to_str.bytes.to_a
  assert_equal([161, 97], array)
end

assert('MessagePack.pack', 'FixRaw "1"') do
  msg = MessagePack.pack("1")
  array = msg.to_str.bytes.to_a
  assert_equal([161, 49], array)
end

assert('MessagePack.pack', 'FixRaw "a" * 31') do
  msg = MessagePack.pack("a" * 31)
  array = msg.to_str.bytes.to_a
  assert_equal([191, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
                97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
                97, 97, 97, 97, 97, 97, 97, 97], array)
end

assert('MessagePack.pack', 'FixArray []') do
  msg = MessagePack.pack([])
  array = msg.to_str.bytes.to_a
  assert_equal([144], array)
end

assert('MessagePack.pack', 'FixArray [1,2,3]') do
  msg = MessagePack.pack([1,2,3])
  array = msg.to_str.bytes.to_a
  assert_equal([147, 1, 2, 3], array)
end

assert('MessagePack.pack', 'FixArray ["a", :b, 3]') do
  msg = MessagePack.pack(["a", :b, 3])
  array = msg.to_str.bytes.to_a
  assert_equal([147, 161, 97, 161, 98, 3], array)
end

assert('MessagePack.pack', 'FixMap {}') do
  msg = MessagePack.pack({})
  array = msg.to_str.bytes.to_a
  assert_equal([128], array)
end

assert('MessagePack.pack', 'FixMap {"a" => 1}') do
  msg = MessagePack.pack({"a" => 1})
  array = msg.to_str.bytes.to_a
  assert_equal([129, 161, 97, 1], array)
end

assert('Packer', 'fixnum 1') do
  packer = MessagePack::Packer.new
  packer.write(1)
  assert_equal(packer.to_str.bytes.to_a[0], 1)
end


# End -- MessagePack-mruby --

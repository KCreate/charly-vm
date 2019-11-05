// Buffer Methods
const __buffer_create        = Charly.internals.get_method("Buffer::create")
const __buffer_reserve       = Charly.internals.get_method("Buffer::reserve")
const __buffer_get_size      = Charly.internals.get_method("Buffer::get_size")
const __buffer_get_offset    = Charly.internals.get_method("Buffer::get_offset")
const __buffer_write         = Charly.internals.get_method("Buffer::write")
const __buffer_str           = Charly.internals.get_method("Buffer::str")

/*
 * Represents a buffer
 *
 * Buffers are regularly allocated memory blocks provided by the VM
 * */
class Buffer {
  property cp
  property size
  property offset

  func constructor(size) {
    @cp = __buffer_create(size)
    @size = size
    @offset = 0
  }

  func reserve(size) {
    __buffer_reserve(@cp, size)
    @size = @get_size()
    @offset = @get_offset()
  }

  func get_size = __buffer_get_size(@cp)
  func get_offset = __buffer_get_offset(@cp)

  func write(src) {
    @offset = __buffer_write(@cp, src)
    @size = @get_size()
  }

  func str {
    __buffer_str(@cp)
  }
}

export = Buffer

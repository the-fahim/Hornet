const box = {value: 42, read: function () { return this.value; }};
const detached = box.read;
// A bare function call has an undefined receiver in Hornet.
detached();

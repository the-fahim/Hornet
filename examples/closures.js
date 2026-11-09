// Closures share captured bindings, while separate calls create separate state.
function makeAccount(balance) {
    return {
        deposit: function (amount) {
            balance = balance + amount;
            return balance;
        },
        read: function () { return balance; }
    };
}
const first = makeAccount(10);
const second = makeAccount(100);
console.log(first.deposit(5), first.read(), second.read());

// A fresh block binding on each iteration survives after the block exits.
const readers = {};
let i = 0;
while (i < 3) {
    const snapshot = i;
    readers[i] = function () { return snapshot; };
    i = i + 1;
}
console.log(readers[0](), readers[1](), readers[2]());

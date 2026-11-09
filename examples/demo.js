// Hornet compiles this entire program to bytecode, then executes it in its VM.
console.log("Hornet");

function sum(n) {
    let result = 0;
    let i = 0;
    while (i < n) {
        result = result + i;
        i = i + 1;
    }
    return result;
}
console.log(sum(100));

const box = {
    value: 40,
    add: function (amount) {
        this.value = this.value + amount;
        return this.value;
    }
};
console.log(box.add(2));

function makeCounter() {
    let count = 0;
    return function () {
        count = count + 1;
        return count;
    };
}
const counter = makeCounter();
console.log(counter(), counter());
console.log(box.value === 42);

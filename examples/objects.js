// const protects the variable binding, but the object can still change.
const counter = {
    value: 0,
    step: function (amount) {
        this.value = this.value + amount;
        return this.value;
    }
};
const alias = counter;
console.log(counter.step(2), alias["step"](3), counter.value);
console.log(alias === counter, {} === {}, counter.missing);

// Methods use their current receiver, not the object that first held them.
const other = {value: 10, step: counter.step};
console.log(other.step(4), counter.value);
counter.self = counter;
console.log(counter.self === counter);

// Primitive keys become strings; later writes replace existing properties.
counter[true] = "boolean key";
counter[null] = "null key";
console.log(counter["true"], counter["null"]);

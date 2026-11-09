// Function declarations can be called before their declaration.
console.log(double(21));
function double(n) { return n * 2; }

let name = "outer";
{
    let name = "inner";
    console.log(name);
}
console.log(name);

// An escaped closure keeps its lexical scope, not its caller's scope.
function makeReader() {
    const name = "captured";
    return function () { return name; };
}
const reader = makeReader();
function callReader() {
    const name = "caller";
    return reader();
}
console.log(callReader());

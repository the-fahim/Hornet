let trace = "";
function mark(label, value) {
    trace = trace + label;
    return value;
}
const object = {value: 0};
function receiver() { trace = trace + "object "; return object; }
function key() { trace = trace + "key "; return "value"; }
receiver()[key()] = mark("rhs", 42);
console.log(trace, object.value);

trace = "";
console.log(mark("left ", 2) + mark("right", 3), trace);

// Neither skipped expression runs. Logical operators return an operand.
trace = "";
false && mark("skipped", 1);
true || mark("skipped", 2);
console.log(trace === "", "" || "fallback", "ready" && 42);

// The callee is captured before evaluating arguments that replace it.
let chosen = function () { return "original"; };
function replace() { chosen = function () { return "replacement"; }; }
console.log(chosen(replace()), chosen());

// Euclid's algorithm uses a loop and remainder.
function gcd(a, b) {
    while (b !== 0) {
        const remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}
console.log(gcd(48, 18), gcd(17, 13), gcd(0, 5));

// Returning inside nested blocks exits the entire function.
function findFirstMultiple(limit, divisor) {
    let n = 1;
    while (n <= limit) {
        if (n % divisor === 0) { return n; }
        n = n + 1;
    }
    return null;
}
console.log(findFirstMultiple(20, 7), findFirstMultiple(3, 7));

function classify(n) {
    if (n < 0) return "negative";
    else if (n === 0) return "zero";
    else return "positive";
}
console.log(classify(-3), classify(0), classify(3));

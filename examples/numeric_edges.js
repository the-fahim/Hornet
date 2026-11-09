// Floating-point values include signed zero, infinity, and NaN.
console.log(1 / 0, 1 / -0, 0 === -0);
console.log(0 / 0, Infinity - Infinity, 0 * Infinity);
console.log(NaN === NaN, NaN !== NaN, NaN < 1, NaN >= 1);
console.log(-7 % 3, 7 % -3, 7 % Infinity);
console.log(0.1 + 0.2 === 0.3, 9007199254740992 === 9007199254740993);
// Underflow preserves the sign of zero, even though printing zero hides it.
console.log(1 / 1e-999, 1 / -1e-999);

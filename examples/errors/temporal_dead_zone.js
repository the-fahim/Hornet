let value = "outer";
{
    // The inner binding exists here, but has not been initialized yet.
    console.log(value);
    let value = "inner";
}

const settings = {enabled: true};
settings.enabled = false; // Mutating the object is allowed.
settings = {}; // Replacing the const binding raises TypeError.

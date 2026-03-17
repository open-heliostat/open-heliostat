<script lang="ts">
    export let label: string;
    export let value: number;
    export let min = 0;
    export let max = 1;
    export let step = 0.01;
    export let onChange = () => {};
    export let hasNumber = true;
    export let disabled = false;
    export let strictNumberBounds = true;
    // $: value = Math.round(value/step)*step;
</script>

<label class="label cursor-pointer" for={label}>
    <span class="mr-4">{label}</span>
</label>
<div class="flex w-full items-center gap-3">
    <input
        type="range"
        min={min}
        max={max}
        step={step}
        class={"range flex-1 " + (disabled == false ? "range-primary" : "range-secondary")}
        id={label}
        {disabled}
        bind:value={value}
        on:change={onChange}
    />
    {#if hasNumber}
    <input
        type="number"
        min={strictNumberBounds ? min : undefined}
        max={strictNumberBounds ? max : undefined}
        step={step}
        class="input input-bordered h-8 w-20"
        id={label}
        {disabled}
        bind:value={value}
        on:change={onChange}
    />
    {/if}
</div>
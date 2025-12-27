<script lang="ts">
	import Stop from '~icons/tabler/hand-stop';
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import type { StepperControl, StepperDiag } from '$lib/types/models';

	export let stepperControl: StepperControl;
	export let onChange: () => void;

</script>

<style>
	.grid-form {
	  grid-template-columns: [labels] auto [controls] 1fr;
	}
</style>

<div class="w-full grid grid-flow-row grid-form items-center">
	<Checkbox
		label="Enable"
		bind:value={stepperControl.isEnabled}
		{onChange}
	></Checkbox>
	<Checkbox
		label="Direction"
		bind:value={stepperControl.direction}
		{onChange}
	></Checkbox>
	<Slider
		label="Speed"
		min={0}
		max={1}
		step={0.01}
		bind:value={stepperControl.speed}
		{onChange}
	></Slider>
	<Slider
		label="Move"
		min={0}
		max={1}
		step={0.01}
		bind:value={stepperControl.move}
		{onChange}
	></Slider>
	<Slider
		label="Acceleration"
		min={0}
		max={1}
		step={0.01}
		bind:value={stepperControl.acceleration}
		{onChange}
	></Slider>
</div>
<div class="flex flex-row flex-wrap justify-between gap-x-2">
	<div class="flex-grow"></div>
	<div>
		<div>
			<button class="btn btn-primary inline-flex items-center" on:click={() => {stepperControl.speed=0; onChange();}}
				><Stop class="mr-2 h-5 w-5" /><span>Stop</span></button
			>
		</div>
	</div>
</div>
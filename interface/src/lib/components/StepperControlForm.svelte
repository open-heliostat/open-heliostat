<script lang="ts">
	import Stop from '~icons/tabler/hand-stop';
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import type { StepperControlState, StepperDiag } from '$lib/types/models';
	import { postJsonRest, getJsonRest} from "$lib/stores/rest"
	import { onDestroy, onMount } from "svelte";
	import StopButton from './StopButton.svelte';
	import DisableButton from './DisableButton.svelte';

	export let restPath : string;
	let stepperControl : StepperControlState;

	let intervalID : any = null;
	onMount(() => {
		intervalID = setInterval(() => {
			getStepperControl();
		}, 2000);
	});
	onDestroy(() => {
		clearInterval(intervalID);
	});

	async function getStepperControl() {
		return getJsonRest(restPath, stepperControl).then(data => stepperControl = data);
	}
	async function postStepperControl(control: StepperControlState) {
		return postJsonRest(restPath, control);
	}


</script>

<style>
	.grid-form {
	  grid-template-columns: [labels] auto [controls] 1fr;
	}
</style>

{#await getStepperControl() then nothing}
<div class="w-full grid grid-flow-row grid-form items-center">
	<!-- <Checkbox
		label="Enable"
		bind:value={stepperControl.isEnabled}
		{onChange}
	></Checkbox>
	<Checkbox
		label="Direction"
		bind:value={stepperControl.direction}
		{onChange}
	></Checkbox> -->
	<Slider
		label="Speed"
		min={-1.}
		max={1.}
		step={0.01}
		bind:value={stepperControl.speed}
		onChange={()=>{postStepperControl(stepperControl)}}
	></Slider>
	<Slider
		label="Move"
		min={-180.}
		max={180.}
		step={0.1}
		bind:value={stepperControl.move}
		onChange={()=>{postJsonRest(restPath, {move: stepperControl.move})}}
	></Slider>
	<Slider
		label="Acceleration"
		min={0}
		max={1}
		step={0.01}
		bind:value={stepperControl.accel}
		onChange={()=>{postStepperControl(stepperControl)}}
	></Slider>
</div>
{/await}
<slot></slot>
<div class="flex flex-row flex-wrap justify-between gap-x-2">
	<div class="flex-grow"></div>
	<div>
		<div>
			<DisableButton onClick={() => postJsonRest(restPath, {enabled: false})}></DisableButton>
			<StopButton onClick={() => {stepperControl.speed=0; postStepperControl(stepperControl);}}></StopButton>
		</div>
	</div>
</div>

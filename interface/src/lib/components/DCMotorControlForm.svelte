<script lang="ts">
	import Slider from '$lib/components/Slider.svelte';
	import type { DCMotorControlState } from '$lib/types/models';
	import { postJsonRest, getJsonRest} from "$lib/stores/rest"
	import { onDestroy, onMount } from "svelte";
	import StopButton from './StopButton.svelte';
	import Checkbox from './Checkbox.svelte';
	import Collapsible from './Collapsible.svelte';

	export let restPath : string;
	let motorControl : DCMotorControlState;

	let intervalID : any = null;
	onMount(() => {
		intervalID = setInterval(() => {
			getMotorControl();
		}, 2000);
	});
	onDestroy(() => {
		clearInterval(intervalID);
	});

	async function getMotorControl() {
		return getJsonRest(restPath, motorControl).then(data => motorControl = data);
	}
	async function postMotorControl(control: DCMotorControlState) {
		return postJsonRest(restPath, control);
	}
</script>

{#await getMotorControl() then nothing}
<div class="w-full grid grid-flow-row grid-form items-center">
	<Slider
		label="Speed"
		min={-1.}
		max={1.}
		step={0.01}
		bind:value={motorControl.speed}
		onChange={()=>{postJsonRest(restPath, {speed: motorControl.speed})}}
	></Slider>
    <!-- <Slider
        label="Duty Cycle"
        min={0}
        max={100}
        step={1}
        bind:value={motorControl.duty}
        onChange={()=>{postMotorControl(motorControl)}}
    ></Slider>
    <Checkbox
        label="Direction"
        bind:value={motorControl.direction}
        onChange={()=>{postMotorControl(motorControl)}}
    ></Checkbox> -->
</div>
{/await}
<Collapsible>
	{#snippet title()}
		<span>Settings</span>
	{/snippet}
	<slot></slot>
</Collapsible>
<div class="flex flex-row flex-wrap justify-between gap-x-2">
	<div class="flex-grow"></div>
	<div>
		<div>
			<StopButton onClick={() => {motorControl.speed=0; postJsonRest(restPath, {speed: motorControl.speed});}}></StopButton>
		</div>
	</div>
</div>

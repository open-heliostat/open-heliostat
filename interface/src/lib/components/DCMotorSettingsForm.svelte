<script lang="ts">
	import Slider from '$lib/components/Slider.svelte';
	import type { DCMotorSettings } from '$lib/types/models';
	import { postJsonRest, getJsonRest} from "$lib/stores/rest"
	import { onDestroy, onMount } from "svelte";
	import Checkbox from './Checkbox.svelte';

	export let restPath : string;
	let motorSettings : DCMotorSettings;

	let intervalID : any = null;
	onMount(() => {
		intervalID = setInterval(() => {
			getmotorSettings();
		}, 2000);
	});
	onDestroy(() => {
		clearInterval(intervalID);
	});

	async function getmotorSettings() {
		return getJsonRest(restPath, motorSettings).then(data => motorSettings = data);
	}
	async function postmotorSettings(control: DCMotorSettings) {
		return postJsonRest(restPath, control);
	}
</script>

<style>
	.grid-form {
	  grid-template-columns: [labels] auto [controls] 1fr;
	}
</style>

{#await getmotorSettings() then nothing}
<div class="w-full grid grid-flow-row grid-form items-center">
	<Checkbox
        label="Invert Direction"
        bind:value={motorSettings.invertDirection}
        onChange={()=>{postmotorSettings(motorSettings)}}
    ></Checkbox>
    <Slider
        label="Shape"
        min={0}
        max={1}
        step={0.01}
        bind:value={motorSettings.shape}
        onChange={()=>{postmotorSettings(motorSettings)}}
    ></Slider>
    <Slider
        label="Min Value"
        min={0}
        max={1}
        step={0.01}
        bind:value={motorSettings.minVal}
        onChange={()=>{postmotorSettings(motorSettings)}}
    ></Slider>
</div>
{/await}
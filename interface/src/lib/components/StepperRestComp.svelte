<script lang="ts">
	import StepperControlForm from "./StepperControlForm.svelte";
	import StepperSettingsForm from "./StepperSettingsForm.svelte";
    import StepperStatusComp from "./StepperStatusComp.svelte";

	import type { StepperConfig, StepperControlState, StepperDiag } from '$lib/types/models'
	import { postJsonRest, getJsonRest} from "$lib/stores/rest"
	import { onDestroy, onMount } from "svelte";

	export let restPath : string;
	
	let stepperControl : StepperControlState;
	let stepperDiag : StepperDiag;
	let stepperConfig : StepperConfig;

	let intervalID : any = null;
	onMount(() => {
		intervalID = setInterval(() => {
			getStepperDiag();
		}, 5000);
	});
	onDestroy(() => {
		clearInterval(intervalID);
	});

	async function getStepperControl() {
		return getJsonRest(restPath + '/control', stepperControl).then(data => stepperControl = data);
	}
	async function postStepperControl(control: StepperControlState) {
		return postJsonRest(restPath + '/control', control);
	}
	async function getStepperConfig() {
		return getJsonRest(restPath + '/config', stepperConfig).then(data => stepperConfig = data);
	}
	async function postStepperConfig(config: StepperConfig) {
		return postJsonRest(restPath + '/config', config).then(data => stepperConfig = data);
	}
	async function getStepperDiag() {
		return getJsonRest(restPath + '/diag', stepperDiag).then(data => stepperDiag = data);
	}

</script>

{#await getStepperDiag() then nothing} 
	<StepperStatusComp 
		stepperControl={stepperDiag}
		enableCb={()=>{postJsonRest(restPath + '/config', {enabled: true})}}
	></StepperStatusComp>
{/await}
<StepperControlForm 
	restPath={restPath + "/control"}
	>
	{#await getStepperConfig() then nothing} 
	<StepperSettingsForm 
		stepperSettings={stepperConfig}
		onChange={()=>{postStepperConfig(stepperConfig).then((data) => stepperConfig = data)}}
	></StepperSettingsForm>
	{/await}
</StepperControlForm>